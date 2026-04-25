---
type: guide
status: completed
last_updated: 2026-04-25
---

# Firmware TFLite Integration Guide

Tags: #tflite #ai #driver #tinyml

## Overview
Tài liệu này vạch ra quy trình chuẩn để tích hợp thư viện **TensorFlow Lite cho Microcontrollers (TFLite Micro)** vào các dự án baremetal hoặc RTOS trên vi điều khiển Renesas RA. Tài liệu này đóng vai trò là sách hướng dẫn cho việc triển khai các mô hình Machine Learning mới trong tương lai và cung cấp cẩm nang theo dõi (trace) các lỗi phổ biến.

---

## 1. Directory Structure Setup (Cấu trúc thư mục)
Khi muốn đưa TFLite Micro sang một dự án mới:
- **KHÔNG** copy toàn bộ clone của `tflite-micro` repo.
- **HÃY** copy một phiên bản "portable" đã được lược bỏ các thành phần thừa. Chỉ sao chép các thư mục: `tensorflow/lite/micro/`, `tensorflow/lite/c/`, `tensorflow/lite/core/`, `tensorflow/lite/schema/`, và `third_party/`.
- **RẤT QUAN TRỌNG:** Phải xóa tất cả các file test (`*_test.cc`, `*_test.c`) để ngăn CMake `GLOB_RECURSE` biên dịch hàng tá hàm `main()` gây lỗi multiple definition.

---

## 2. Implementing the Inference Wrapper (Triển khai Wrapper)
Luôn đóng gói các API C++ của TFLite Micro bên trong một thư viện bọc (wrapper) bằng `extern "C"`. Điều này đảm bảo code inference có thể được gọi an toàn từ mã nguồn C chuẩn như `main.c` hay các RTOS Tasks.

### Các thành phần cốt lõi:
1. `tflite::GetModel(g_model_data)`: Đọc và xác thực cấu trúc mô hình dạng mảng.
2. `tflite::AllOpsResolver`: Nạp toàn bộ các toán tử nơ-ron có thể dùng. 
   - *Tối ưu:* Để tiết kiệm bộ nhớ flash, có thể dùng `MicroMutableOpResolver` và chỉ `Add()` những toán tử mô hình bạn dùng (vd: AddConv1D, AddDense).
3. `tflite::MicroInterpreter`: Đối tượng chính thực thi mô hình.
4. `tensor_arena`: Mảng tĩnh bằng `uint8_t` phục vụ cấp phát vùng nhớ trung gian cho TFLite. Bắt buộc phải có `alignas(16)`.

### Mã nguồn mẫu (Boilerplate)
```cpp
constexpr int kTensorArenaSize = 16 * 1024;
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

// Initialize
model = tflite::GetModel(g_model_data);
static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
interpreter = &static_interpreter;
interpreter->AllocateTensors();

// Predict
input->data.f[i] = value;
interpreter->Invoke();
return output->data.f[0];
```

---

## 3. Pre-processing & Feature Scaling (Tiền xử lý dữ liệu)
Các mô hình huấn luyện bằng Python (sử dụng Scikit-learn như `StandardScaler` hay `MinMaxScaler`) yêu cầu dữ liệu phải được scale tại vi điều khiển **trước khi** đưa vào mô hình.

- Định nghĩa sẵn bộ hằng số `MEAN` (Trung bình) và `SCALE` (Độ lệch chuẩn) (vd: `Scaler_Constants.h`).
- Sử dụng Ring Buffer hoặc thao tác dịch mảng (Shift Buffer) nếu mô hình yêu cầu chuỗi thời gian lịch sử (VD: cần 6 giờ đọc cảm biến trước đó).
- Scale dữ liệu theo công thức: `input = (raw_value - MEAN) / SCALE` và gán vào tensor.

---

## 4. Common Errors & Tracing (Lỗi phổ biến & Cách gỡ lỗi)
Sử dụng hàm `MicroPrintf()` (hoặc cấu hình lại `#define TF_LITE_STRIP_ERROR_STRINGS 0`) để xem lỗi khi chạy debug.

| Lỗi gặp phải | Nguyên nhân & Cách khắc phục |
|-------------|-----------------------------|
| **`AllocateTensors() failed`** | Vùng nhớ `kTensorArenaSize` cấp phát quá nhỏ không đủ lưu tensor trung gian. Tăng kích thước lên 2KB hoặc 4KB mỗi lần cho đến khi thành công. Xem file .map để cân đối lại vùng RAM. |
| **`Model schema version mismatch`** | Phiên bản Python converter tạo file tflite không đồng bộ với phiên bản thư viện TFLite C++ Micro đang chạy. Cần convert lại model. |
| **`Input/Output type mismatch`** | Thường xảy ra khi model bị quantized (đưa về `int8`) nhưng mã C lại trích xuất bằng mảng `float` (hoặc ngược lại). Kiểm tra thông số `optimizations` khi convert model ở python. |
| **`Invoke failed / Missing Ops`** | Nếu dùng `MicroMutableOpResolver` nhưng quên `Add()` toán tử (operation) nào đó. Thay tạm bằng `AllOpsResolver` để xác nhận. |
| **HardFault ngay khi khởi động** | Quên từ khóa `alignas(16)` khi định nghĩa `tensor_arena`. Vi điều khiển Cortex-M lỗi alignment truy cập bộ nhớ. |

---

## 5. Build System (CMake)
- Đảm bảo Compiler Flags có các cờ: `-std=c++14`, `-fno-rtti`, `-fno-exceptions`. TFLite Micro được thiết kế không dùng Runtime Type Information (RTTI) và C++ Exceptions để tiết kiệm RAM.
- Add thư mục thư viện vào `include_directories()`, đặc biệt chú ý đường dẫn tới `third_party/flatbuffers/include`.
