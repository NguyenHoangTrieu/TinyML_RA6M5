from contextlib import asynccontextmanager
import uvicorn
from fastapi import FastAPI, BackgroundTasks, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from backend.database_manager import db_manager
from backend.mqtt_client import mqtt_service
import threading
import os

# --- 1. QUẢN LÝ VÒNG ĐỜI (THAY THẾ CHO on_event) ---
@asynccontextmanager
async def lifespan(app: FastAPI):
    # Logic chạy khi ứng dụng Startup
    print("🚀 Khởi tạo Database...")
    db_manager.initialize_db()
    
    print("📡 Đang khởi động MQTT Client Service...")
    mqtt_thread = threading.Thread(target=mqtt_service.run, daemon=True)
    mqtt_thread.start()
    
    yield # Ứng dụng API sẽ hoạt động trong suốt quá trình này
    
    # Logic chạy khi ứng dụng Shutdown (Tắt server)
    print("🛑 Đang dọn dẹp và tắt hệ thống...")

# --- 2. KHỞI TẠO APP VỚI LIFESPAN ---
app = FastAPI(
    title="ZMOD4410 Edge AI Gateway",
    description="Backend xử lý chỉ số IAQ theo chuẩn UBA",
    version="2.0.0",
    lifespan=lifespan  # Kích hoạt Lifespan
)

# Cấu hình CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- 3. CÁC API ENDPOINTS ---
@app.get("/")
async def root():
    return {"message": "IAQ Edge AI Backend is running", "status": "online"}

@app.get("/api/v1/latest")
async def get_latest_data():
    latest = db_manager.get_latest_record()
    if latest:
        return latest
    return {"error": "No data available"}

@app.get("/api/v1/history")
async def get_history(limit: int = 100):
    data = db_manager.get_history(limit)
    return data

@app.post("/api/v1/retrain")
async def trigger_retrain(background_tasks: BackgroundTasks):
    from ai_engine.retraining_script import run_retraining
    background_tasks.add_task(run_retraining)
    return {"message": "Retraining process started in background"}

# ── Model serving endpoints (consumed by ESP32-C6 OTA updater) ──────────────

TFLITE_MODEL_PATH = "updates/iaq_model.tflite"

@app.get("/api/v1/model/version")
async def get_model_version():
    """
    Return the current model version token (file size in bytes).
    The ESP32-C6 polls this endpoint every 60 s and compares against its
    last-downloaded size to detect when a new retrained model is available.
    Returns {"version": 0} when no model has been produced yet.
    """
    if not os.path.exists(TFLITE_MODEL_PATH):
        return {"version": 0}
    return {"version": os.path.getsize(TFLITE_MODEL_PATH)}

@app.get("/api/v1/model/latest")
async def get_model_latest():
    """
    Serve the latest retrained TFLite model binary.
    The ESP32-C6 downloads this binary and pushes it to the RA6M5 over UART
    using the fwupdate framed protocol. The RA6M5 writes it to Data Flash
    and uses it for IAQ inference on the next (and all subsequent) boots.
    """
    if not os.path.exists(TFLITE_MODEL_PATH):
        raise HTTPException(status_code=404, detail="No retrained model available yet")
    return FileResponse(
        path=TFLITE_MODEL_PATH,
        media_type="application/octet-stream",
        filename="iaq_model.tflite",
    )

# --- 4. CHẠY SERVER ---
if __name__ == "__main__":
    # ĐÃ SỬA: Chỉ định rõ đường dẫn "backend.main:app" thay vì "main:app"
    uvicorn.run("backend.main:app", host="0.0.0.0", port=8000, reload=True)