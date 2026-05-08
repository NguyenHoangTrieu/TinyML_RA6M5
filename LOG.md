=== RA6M5 RTOS Application Test ===
[0 ms] UART  : SCI7 P613/P614 @ 115200 baud
[0 ms] I2C   : RIIC1 P512(SCL)/P511(SDA) @ 100 kHz
[0 ms] TDRE  : OK
[0 ms] I2C scan (0x08-0x77):
[0 ms]   No devices found.
[0 ms]   Check pull-ups, power, and SCL/SDA routing.
[0 ms] 
[0 ms] [USB TEST] Mode: Device CDC logger task
[0 ms] [FLASH NVS TEST] Mode: scratch-block self-test
[0 ms] Tasks ready: LED1 blink, AHT20 logger, LED2 timer blink
[0 ms] [USB CDC TEST] Device mode active. Connect USB FS to host.
[5 ms] [USB CDC TRACE] state=BUS_RESET_REARM v0=0x40 v1=0x0 v2=0x40
[11 ms] [USB CDC TRACE] init mode=0 SYSCFG=0x401 INTENB0=0x9f00 BRDYENB=0x1
[18 ms] [sensor:0] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[23 ms] 
[RTOS TEST] Starting Preemptive Test...
[28 ms] [USB CDC TRACE] state=PULLUP_ENABLED v0=0x411 v1=0xa v2=0x80

[34 ms] [[35 ms] [USB CDC TRACE] DVST is0=0x10c0 SYSSTS0=0x1 DVSTCTR0=0x0 addr=0 cfg=0
RTOS TEST] Preemptive Scheduling: PASS
[166 ms] [USB CDC TRACE] DVST is0=0x5090 SYSSTS0=0x0 DVSTCTR0=0x2 addr=0 cfg=0
[173 ms] [USB CDC TRACE] state=BUS_RESET_REARM v0=0x40 v1=0x0 v2=0x40
[213 ms] [USB CDC TRACE] CTRT is0=0x6899 intsts0=0x6099 CTSQ=RDDS(1) pending=0
[220 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x100 wIndex=0x0 wLen=64
[228 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x100 v1=0x12 v2=0x0
[234 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x12 v1=0x0 v2=0x12
[241 ms] [USB CDC TRACE] CTRT is0=0x6892 intsts0=0x6092 CTSQ=RDSS(2) pending=0
[248 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[255 ms] [USB CDC TRACE] DVST is0=0x7890 SYSSTS0=0x1 DVSTCTR0=0x2 addr=0 cfg=0
[262 ms] [USB CDC TRACE] state=BUS_RESET_REARM v0=0x8000 v1=0x0 v2=0x40
[268 ms] [USB CDC TRACE] CTRT is0=0x6890 intsts0=0x6090 CTSQ=IDST(0) pending=0
[289 ms] [USB CDC TRACE] DVST is0=0x70a8 SYSSTS0=0x1 DVSTCTR0=0x2 addr=0 cfg=0
[299 ms] [USB CDC TRACE] CTRT is0=0x68a9 intsts0=0x60a9 CTSQ=RDDS(1) pending=0
[306 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x100 wIndex=0x0 wLen=18
[314 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x100 v1=0x12 v2=0x0
[320 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x12 v1=0x0 v2=0x12
[327 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[334 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[341 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[348 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=255
[356 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x43 v2=0x0
[363 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x40 v1=0x3 v2=0x43
[369 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x60a1 CTSQ=RDDS(1) pending=0
[376 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=255
[384 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x43 v2=0x0
[391 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x40 v1=0x3 v2=0x43
[398 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x3 v1=0x0 v2=0x3
[404 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[411 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[418 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[425 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x303 wIndex=0x409 wLen=255
[434 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x303 v1=0x1c v2=0x0
[440 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x1c v1=0x0 v2=0x1c
[446 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x60a1 CTSQ=RDDS(1) pending=0
[453 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x303 wIndex=0x409 wLen=255
[462 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x303 v1=0x1c v2=0x0
[468 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x1c v1=0x0 v2=0x1c
[474 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[481 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x1
[488 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[495 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x300 wIndex=0x0 wLen=255
[504 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x300 v1=0x4 v2=0x0
[510 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x4 v1=0x0 v2=0x4
[516 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x68a2 CTSQ=RDSS(2) pending=0
[523 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[529 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x68a0 CTSQ=IDST(0) pending=0
[536 ms] [USB CDC TRACE] CTRT is0=0x68a0 intsts0=0x60a0 CTSQ=IDST(0) pending=0
[544 ms] [USB CDC TRACE] CTRT is0=0x68a9 intsts0=0x60a9 CTSQ=RDDS(1) pending=0
[551 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x302 wIndex=0x409 wLen=255
[559 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x302 v1=0x26 v2=0x0
[566 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x26 v1=0x0 v2=0x26
[573 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[580 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[587 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[594 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x600 wIndex=0x0 wLen=10
[602 ms] [USB CDC TRACE] STALL reason=UNSUPPORTED_STANDARD req=GET_DESCRIPTOR DCPCTR=0x8040 INTSTS0=0x68a1
[612 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x60a1 CTSQ=RDDS(1) pending=0
[619 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x600 wIndex=0x0 wLen=10
[627 ms] [USB CDC TRACE] STALL reason=UNSUPPORTED_STANDARD req=GET_DESCRIPTOR DCPCTR=0x8042 INTSTS0=0x60a1
[638 ms] [USB CDC TRACE] CTRT is0=0x68a9 intsts0=0x60a9 CTSQ=RDDS(1) pending=0
[645 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x100 wIndex=0x0 wLen=18
[653 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x100 v1=0x12 v2=0x0
[659 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x12 v1=0x0 v2=0x12
[666 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[673 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[680 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[687 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=9
[695 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x9 v2=0x0
[701 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x9 v1=0x0 v2=0x9
[707 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x68a2 CTSQ=RDSS(2) pending=0
[714 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[721 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x68a0 CTSQ=IDST(0) pending=0
[728 ms] [USB CDC TRACE] CTRT is0=0x68a0 intsts0=0x60a0 CTSQ=IDST(0) pending=0
[736 ms] [USB CDC TRACE] CTRT is0=0x68a9 intsts0=0x60a9 CTSQ=RDDS(1) pending=0
[743 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=67
[751 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x43 v2=0x0
[757 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x40 v1=0x3 v2=0x43
[764 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x3 v1=0x0 v2=0x3
[770 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[777 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8041
[784 ms] [USB CDC TRACE] CTRT is0=0x68a8 intsts0=0x68a9 CTSQ=RDDS(1) pending=0
[791 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=265
[799 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x43 v2=0x0
[806 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x40 v1=0x3 v2=0x43
[812 ms] [USB CDC TRACE] CTRT is0=0x68a1 intsts0=0x64a1 CTSQ=RDDS(1) pending=0
[819 ms] [USB CDC TRACE] SETUP GET_DESCRIPTOR bmReq=0x80 bReq=0x6 wValue=0x200 wIndex=0x0 wLen=265
[827 ms] [USB CDC TRACE] state=GET_DESCRIPTOR v0=0x200 v1=0x43 v2=0x0
[834 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x40 v1=0x3 v2=0x43
[841 ms] [USB CDC TRACE] state=CTRL_IN_PACKET v0=0x3 v1=0x0 v2=0x3
[847 ms] [USB CDC TRACE] CTRT is0=0x68a2 intsts0=0x60a2 CTSQ=RDSS(2) pending=0
[854 ms] [USB CDC TRACE] state=STATUS_STAGE CTSQ=RDSS(2) DCPCTR=0x8001
[861 ms] [USB CDC TEST] configured=1
[865 ms] [USB CDC TEST] host_ready=1
[870 ms] 
[IAQ Task] Starting TFLite initialization...
[880 ms] [IAQ Task] Init OK. Starting 5s forecast loop...
[885 ms] [SensorSim_Init OK]
[888 ms] [SensorSim_Read OK]
[891 ms] [IAQ_Predict OK]
[893 ms] Published: TVOC=2755.3ppb | Actual=4.32 | Predict=4.32
[1245 ms] [FLASH NVS TEST] start addr=0x8001fc0 erase=64B raw=21B write=24B
[1253 ms] [FLASH NVS TEST] PASS addr=0x8001fc0 bytes=21
[14289 ms] [USB CDC TEST] write failed (1)
[14293 ms] [sensor:1] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[14299 ms] [SensorSim_Read OK]
[14303 ms] [IAQ_Predict OK]
[14305 ms] Published: TVOC=3110.4ppb | Actual=4.41 | Predict=4.41
[27923 ms] [USB CDC TEST] write failed (1)
[27927 ms] [sensor:2] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[27932 ms] [SensorSim_Read OK]
[27935 ms] [IAQ_Predict OK]
[27938 ms] Published: TVOC=3374.4ppb | Actual=4.48 | Predict=4.47
[28000 ms] [timer] LED2 toggles=10
[41557 ms] [USB CDC TEST] write failed (1)
[41561 ms] [sensor:3] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[41566 ms] [SensorSim_Read OK]
[41569 ms] [IAQ_Predict OK]
[41572 ms] Published: TVOC=3580.4ppb | Actual=4.53 | Predict=4.53
[55191 ms] [USB CDC TEST] write failed (1)
[55195 ms] [sensor:4] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[55200 ms] [SensorSim_Read OK]
[55203 ms] [IAQ_Predict OK]
[55206 ms] Published: TVOC=3895.7ppb | Actual=4.61 | Predict=4.60
[68825 ms] [USB CDC TEST] write failed (1)
[68829 ms] [sensor:5] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[68834 ms] [timer] LED2 toggles=20
[68837 ms] [SensorSim_Read OK]
[68840 ms] [IAQ_Predict OK]
[68843 ms] Published: TVOC=4127.8ppb | Actual=4.67 | Predict=4.67
[82459 ms] [USB CDC TEST] write failed (1)
[82463 ms] [sensor:6] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[82468 ms] [SensorSim_Read OK]
[82471 ms] [IAQ_Predict OK]
[82474 ms] Published: TVOC=4303.6ppb | Actual=4.72 | Predict=4.72
[96093 ms] [USB CDC TEST] write failed (1)
[96097 ms] [sensor:7] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[96102 ms] [SensorSim_Read OK]
[96105 ms] [IAQ_Predict OK]
[96108 ms] Published: TVOC=4564.9ppb | Actual=4.78 | Predict=4.79
[96500 ms] [timer] LED2 toggles=30
[109727 ms] [USB CDC TEST] write failed (1)
[109731 ms] [sensor:8] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[109736 ms] [SensorSim_Read OK]
[109739 ms] [IAQ_Predict OK]
[109742 ms] Published: TVOC=4764.2ppb | Actual=4.83 | Predict=4.84
[123362 ms] [USB CDC TEST] write failed (1)
[123366 ms] [sensor:9] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[123371 ms] [SensorSim_Read OK]
[123374 ms] [IAQ_Predict OK]
[123377 ms] Published: TVOC=4922.1ppb | Actual=4.87 | Predict=4.89
[136997 ms] [USB CDC TEST] write failed (1)
[137001 ms] [sensor:10] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[137006 ms] [timer] LED2 toggles=40
[137009 ms] [SensorSim_Read OK]
[137013 ms] [IAQ_Predict OK]
[137015 ms] Published: TVOC=5079.9ppb | Actual=5.00 | Predict=4.93
[150631 ms] [USB CDC TEST] write failed (1)
[150635 ms] [sensor:11] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[150640 ms] [SensorSim_Read OK]
[150644 ms] [IAQ_Predict OK]
[150646 ms] Published: TVOC=5172.5ppb | Actual=5.00 | Predict=4.96
[164265 ms] [USB CDC TEST] write failed (1)
[164269 ms] [sensor:12] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[164274 ms] [SensorSim_Read OK]
[164278 ms] [IAQ_Predict OK]
[164280 ms] Published: TVOC=5325.8ppb | Actual=5.00 | Predict=4.99
[165001 ms] [timer] LED2 toggles=50
[177900 ms] [USB CDC TEST] write failed (1)
[177904 ms] [sensor:13] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[177909 ms] [SensorSim_Read OK]
[177913 ms] [IAQ_Predict OK]
[177915 ms] Published: TVOC=5367.8ppb | Actual=5.00 | Predict=4.99
[191534 ms] [USB CDC TEST] write failed (1)
[191538 ms] [sensor:14] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[191543 ms] [SensorSim_Read OK]
[191547 ms] [IAQ_Predict OK]
[191549 ms] Published: TVOC=5496.3ppb | Actual=5.00 | Predict=5.00
[205169 ms] [USB CDC TEST] write failed (1)
[205173 ms] [sensor:15] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[205178 ms] [timer] LED2 toggles=60
[205181 ms] [SensorSim_Read OK]
[205185 ms] [IAQ_Predict OK]
[205187 ms] Published: TVOC=5504.2ppb | Actual=5.00 | Predict=5.00
[218804 ms] [USB CDC TEST] write failed (1)
[218808 ms] [sensor:16] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[218813 ms] [SensorSim_Read OK]
[218817 ms] [IAQ_Predict OK]
[218819 ms] Published: TVOC=5468.4ppb | Actual=5.00 | Predict=5.00
[232439 ms] [USB CDC TEST] write failed (1)
[232443 ms] [sensor:17] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[232448 ms] [SensorSim_Read OK]
[232452 ms] [IAQ_Predict OK]
[232454 ms] Published: TVOC=5508.6ppb | Actual=5.00 | Predict=5.00
[233001 ms] [timer] LED2 toggles=70
[246073 ms] [USB CDC TEST] write failed (1)
[246077 ms] [sensor:18] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[246082 ms] [SensorSim_Read OK]
[246086 ms] [IAQ_Predict OK]
[246088 ms] Published: TVOC=5459.7ppb | Actual=5.00 | Predict=5.00
[259708 ms] [USB CDC TEST] write failed (1)
[259712 ms] [sensor:19] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[259717 ms] [SensorSim_Read OK]
[259721 ms] [IAQ_Predict OK]
[259723 ms] Published: TVOC=5314.0ppb | Actual=5.00 | Predict=4.98
[273343 ms] [USB CDC TEST] write failed (1)
[273347 ms] [sensor:20] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[273352 ms] [timer] LED2 toggles=80
[273355 ms] [SensorSim_Read OK]
[273359 ms] [IAQ_Predict OK]
[273361 ms] Published: TVOC=5237.3ppb | Actual=5.00 | Predict=4.97
[286978 ms] [USB CDC TEST] write failed (1)
[286982 ms] [sensor:21] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[286987 ms] [SensorSim_Read OK]
[286991 ms] [IAQ_Predict OK]
[286993 ms] Published: TVOC=5164.5ppb | Actual=5.00 | Predict=4.95
[300612 ms] [USB CDC TEST] write failed (1)
[300616 ms] [sensor:22] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[300621 ms] [SensorSim_Read OK]
[300625 ms] [IAQ_Predict OK]
[300627 ms] Published: TVOC=5005.9ppb | Actual=5.00 | Predict=4.91
[301501 ms] [timer] LED2 toggles=90
[314246 ms] [USB CDC TEST] write failed (1)
[314250 ms] [sensor:23] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[314255 ms] [SensorSim_Read OK]
[314259 ms] [IAQ_Predict OK]
[314261 ms] Published: TVOC=4769.4ppb | Actual=4.84 | Predict=4.84
[327881 ms] [USB CDC TEST] write failed (1)
[327885 ms] [sensor:24] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[327890 ms] [SensorSim_Read OK]
[327894 ms] [IAQ_Predict OK]
[327896 ms] Published: TVOC=4580.3ppb | Actual=4.79 | Predict=4.79
[341515 ms] [USB CDC TEST] write failed (1)
[341519 ms] [sensor:25] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[341524 ms] [timer] LED2 toggles=100
[341527 ms] [SensorSim_Read OK]
[341531 ms] [IAQ_Predict OK]
[341533 ms] Published: TVOC=4396.1ppb | Actual=4.74 | Predict=4.74
[355150 ms] [USB CDC TEST] write failed (1)
[355154 ms] [sensor:26] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[355159 ms] [SensorSim_Read OK]
[355163 ms] [IAQ_Predict OK]
[355165 ms] Published: TVOC=4222.9ppb | Actual=4.70 | Predict=4.69
[368785 ms] [USB CDC TEST] write failed (1)
[368789 ms] [sensor:27] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[368794 ms] [SensorSim_Read OK]
[368798 ms] [IAQ_Predict OK]
[368800 ms] Published: TVOC=3970.8ppb | Actual=4.63 | Predict=4.62
[369501 ms] [timer] LED2 toggles=110
[382420 ms] [USB CDC TEST] write failed (1)
[382424 ms] [sensor:28] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[382429 ms] [SensorSim_Read OK]
[382433 ms] [IAQ_Predict OK]
[382435 ms] Published: TVOC=3699.3ppb | Actual=4.56 | Predict=4.57
[396054 ms] [USB CDC TEST] write failed (1)
[396058 ms] [sensor:29] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[396063 ms] [SensorSim_Read OK]
[396067 ms] [IAQ_Predict OK]
[396069 ms] Published: TVOC=3428.3ppb | Actual=4.49 | Predict=4.49
[409689 ms] [USB CDC TEST] write failed (1)
[409693 ms] [sensor:30] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[409698 ms] [timer] LED2 toggles=120
[409701 ms] [SensorSim_Read OK]
[409705 ms] [IAQ_Predict OK]
[409708 ms] Published: TVOC=3182.7ppb | Actual=4.43 | Predict=4.43
[423324 ms] [USB CDC TEST] write failed (1)
[423328 ms] [sensor:31] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[423333 ms] [SensorSim_Read OK]
[423337 ms] [IAQ_Predict OK]
[423339 ms] Published: TVOC=2928.8ppb | Actual=4.36 | Predict=4.36
[436959 ms] [USB CDC TEST] write failed (1)
[436963 ms] [sensor:32] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[436968 ms] [SensorSim_Read OK]
[436972 ms] [IAQ_Predict OK]
[436974 ms] Published: TVOC=2612.0ppb | Actual=4.28 | Predict=4.28
[437501 ms] [timer] LED2 toggles=130
[450593 ms] [USB CDC TEST] write failed (1)
[450597 ms] [sensor:33] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[450602 ms] [SensorSim_Read OK]
[450606 ms] [IAQ_Predict OK]
[450608 ms] Published: TVOC=2332.9ppb | Actual=4.21 | Predict=4.21
[464227 ms] [USB CDC TEST] write failed (1)
[464231 ms] [sensor:34] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[464236 ms] [SensorSim_Read OK]
[464240 ms] [IAQ_Predict OK]
[464242 ms] Published: TVOC=2082.6ppb | Actual=4.14 | Predict=4.14
[477862 ms] [USB CDC TEST] write failed (1)
[477866 ms] [sensor:35] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[477871 ms] [timer] LED2 toggles=140
[477874 ms] [SensorSim_Read OK]
[477878 ms] [IAQ_Predict OK]
[477880 ms] Published: TVOC=1803.1ppb | Actual=4.07 | Predict=4.07
[491497 ms] [USB CDC TEST] write failed (1)
[491501 ms] [sensor:36] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[491506 ms] [SensorSim_Read OK]
[491510 ms] [IAQ_Predict OK]
[491512 ms] Published: TVOC=1624.9ppb | Actual=4.03 | Predict=4.00
[505131 ms] [USB CDC TEST] write failed (1)
[505135 ms] [sensor:37] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[505140 ms] [SensorSim_Read OK]
[505144 ms] [IAQ_Predict OK]
[505146 ms] Published: TVOC=1373.5ppb | Actual=3.78 | Predict=3.77
[518766 ms] [USB CDC TEST] write failed (1)
[518770 ms] [sensor:38] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[518775 ms] [timer] LED2 toggles=150
[518778 ms] [SensorSim_Read OK]
[518782 ms] [IAQ_Predict OK]
[518784 ms] Published: TVOC=1100.8ppb | Actual=3.54 | Predict=3.52
[532401 ms] [USB CDC TEST] write failed (1)
[532405 ms] [sensor:39] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[532410 ms] [SensorSim_Read OK]
[532414 ms] [IAQ_Predict OK]
[532416 ms] Published: TVOC=905.0ppb | Actual=3.36 | Predict=3.34
[546036 ms] [USB CDC TEST] write failed (1)
[546040 ms] [sensor:40] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[546045 ms] [SensorSim_Read OK]
[546049 ms] [IAQ_Predict OK]
[546051 ms] Published: TVOC=721.6ppb | Actual=3.19 | Predict=3.18
[546501 ms] [timer] LED2 toggles=160
[559671 ms] [USB CDC TEST] write failed (1)
[559675 ms] [sensor:41] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[559680 ms] [SensorSim_Read OK]
[559684 ms] [IAQ_Predict OK]
[559686 ms] Published: TVOC=617.5ppb | Actual=3.10 | Predict=3.09
[573306 ms] [USB CDC TEST] write failed (1)
[573310 ms] [sensor:42] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[573315 ms] [SensorSim_Read OK]
[573319 ms] [IAQ_Predict OK]
[573321 ms] Published: TVOC=468.6ppb | Actual=2.81 | Predict=2.82
[586941 ms] [USB CDC TEST] write failed (1)
[586945 ms] [sensor:43] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[586950 ms] [timer] LED2 toggles=170
[586953 ms] [SensorSim_Read OK]
[586957 ms] [IAQ_Predict OK]
[586960 ms] Published: TVOC=368.5ppb | Actual=2.56 | Predict=2.51
[600575 ms] [USB CDC TEST] write failed (1)
[600579 ms] [sensor:44] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[600584 ms] [SensorSim_Read OK]
[600588 ms] [IAQ_Predict OK]
[600590 ms] Published: TVOC=269.9ppb | Actual=2.30 | Predict=2.20
[614210 ms] [USB CDC TEST] write failed (1)
[614214 ms] [sensor:45] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[614219 ms] [SensorSim_Read OK]
[614223 ms] [IAQ_Predict OK]
[614225 ms] Published: TVOC=117.2ppb | Actual=1.70 | Predict=1.72
[614501 ms] [timer] LED2 toggles=180
[627845 ms] [USB CDC TEST] write failed (1)
[627849 ms] [sensor:46] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[627854 ms] [SensorSim_Read OK]
[627858 ms] [IAQ_Predict OK]
[627860 ms] Published: TVOC=164.0ppb | Actual=2.03 | Predict=1.86
[641480 ms] [USB CDC TEST] write failed (1)
[641484 ms] [sensor:47] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[641489 ms] [SensorSim_Read OK]
[641493 ms] [IAQ_Predict OK]
[641495 ms] Published: TVOC=94.1ppb | Actual=1.56 | Predict=1.64
[655114 ms] [USB CDC TEST] write failed (1)
[655118 ms] [sensor:48] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[655123 ms] [timer] LED2 toggles=190
[655126 ms] [SensorSim_Read OK]
[655130 ms] [IAQ_Predict OK]
[655133 ms] Published: TVOC=153.5ppb | Actual=2.00 | Predict=1.83
[668748 ms] [USB CDC TEST] write failed (1)
[668752 ms] [sensor:49] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[668757 ms] [SensorSim_Read OK]
[668761 ms] [IAQ_Predict OK]
[668763 ms] Published: TVOC=166.5ppb | Actual=2.04 | Predict=1.87
[682383 ms] [USB CDC TEST] write failed (1)
[682387 ms] [sensor:50] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[682392 ms] [SensorSim_Read OK]
[682396 ms] [IAQ_Predict OK]
[682398 ms] Published: TVOC=230.9ppb | Actual=2.20 | Predict=2.07
[682501 ms] [timer] LED2 toggles=200
[696017 ms] [USB CDC TEST] write failed (1)
[696021 ms] [sensor:51] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[696026 ms] [SensorSim_Read OK]
[696030 ms] [IAQ_Predict OK]
[696032 ms] Published: TVOC=268.0ppb | Actual=2.30 | Predict=2.19
[709652 ms] [USB CDC TEST] write failed (1)
[709656 ms] [sensor:52] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[709661 ms] [SensorSim_Read OK]
[709665 ms] [IAQ_Predict OK]
[709667 ms] Published: TVOC=428.9ppb | Actual=2.71 | Predict=2.70
[723287 ms] [USB CDC TEST] write failed (1)
[723291 ms] [sensor:53] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[723296 ms] [timer] LED2 toggles=210
[723299 ms] [SensorSim_Read OK]
[723303 ms] [IAQ_Predict OK]
[723305 ms] Published: TVOC=534.1ppb | Actual=3.03 | Predict=3.01
[736922 ms] [USB CDC TEST] write failed (1)
[736926 ms] [sensor:54] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[736931 ms] [SensorSim_Read OK]
[736935 ms] [IAQ_Predict OK]
[736937 ms] Published: TVOC=726.2ppb | Actual=3.20 | Predict=3.18
[750557 ms] [USB CDC TEST] write failed (1)
[750561 ms] [sensor:55] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[750566 ms] [SensorSim_Read OK]
[750570 ms] [IAQ_Predict OK]
[750572 ms] Published: TVOC=860.4ppb | Actual=3.32 | Predict=3.30
[751001 ms] [timer] LED2 toggles=220
[764192 ms] [USB CDC TEST] write failed (1)
[764196 ms] [sensor:56] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[764201 ms] [SensorSim_Read OK]
[764205 ms] [IAQ_Predict OK]
[764207 ms] Published: TVOC=1113.0ppb | Actual=3.55 | Predict=3.53
[777827 ms] [USB CDC TEST] write failed (1)
[777831 ms] [sensor:57] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[777836 ms] [SensorSim_Read OK]
[777840 ms] [IAQ_Predict OK]
[777842 ms] Published: TVOC=1276.6ppb | Actual=3.69 | Predict=3.68
[788430 ms] [USB CDC TEST] write failed (1)
[788434 ms] [sensor:58] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[788440 ms] [timer] LED2 toggles=230
[788443 ms] [SensorSim_Read OK]
[788446 ms] [IAQ_Predict OK]
[788449 ms] Published: TVOC=1496.0ppb | Actual=3.89 | Predict=3.88
[810623 ms] [USB CDC TEST] write failed (1)
[810627 ms] [sensor:59] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[810633 ms] [SensorSim_Read OK]
[810637 ms] [IAQ_Predict OK]
[810640 ms] Published: TVOC=1802.8ppb | Actual=4.07 | Predict=4.07
[824258 ms] [USB CDC TEST] write failed (1)
[824262 ms] [sensor:60] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[824267 ms] [SensorSim_Read OK]
[824271 ms] [IAQ_Predict OK]
[824273 ms] Published: TVOC=1998.2ppb | Actual=4.12 | Predict=4.12
[824501 ms] [timer] LED2 toggles=240
[837893 ms] [USB CDC TEST] write failed (1)
[837897 ms] [sensor:61] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[837902 ms] [SensorSim_Read OK]
[837906 ms] [IAQ_Predict OK]
[837908 ms] Published: TVOC=2330.2ppb | Actual=4.21 | Predict=4.21
[851528 ms] [USB CDC TEST] write failed (1)
[851532 ms] [sensor:62] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[851537 ms] [SensorSim_Read OK]
[851541 ms] [IAQ_Predict OK]
[851543 ms] Published: TVOC=2581.1ppb | Actual=4.27 | Predict=4.27
[865163 ms] [USB CDC TEST] write failed (1)
[865167 ms] [sensor:63] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[865172 ms] [timer] LED2 toggles=250
[865175 ms] [SensorSim_Read OK]
[865179 ms] [IAQ_Predict OK]
[865182 ms] Published: TVOC=2844.2ppb | Actual=4.34 | Predict=4.34
[878798 ms] [USB CDC TEST] write failed (1)
[878802 ms] [sensor:64] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[878807 ms] [SensorSim_Read OK]
[878811 ms] [IAQ_Predict OK]
[878813 ms] Published: TVOC=3097.2ppb | Actual=4.41 | Predict=4.40
[892433 ms] [USB CDC TEST] write failed (1)
[892437 ms] [sensor:65] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[892442 ms] [SensorSim_Read OK]
[892446 ms] [IAQ_Predict OK]
[892448 ms] Published: TVOC=3380.6ppb | Actual=4.48 | Predict=4.47
[892501 ms] [timer] LED2 toggles=260
[906068 ms] [USB CDC TEST] write failed (1)
[906072 ms] [sensor:66] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[906077 ms] [SensorSim_Read OK]
[906081 ms] [IAQ_Predict OK]
[906083 ms] Published: TVOC=3649.0ppb | Actual=4.55 | Predict=4.55
[919703 ms] [USB CDC TEST] write failed (1)
[919707 ms] [sensor:67] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[919712 ms] [SensorSim_Read OK]
[919716 ms] [IAQ_Predict OK]
[919718 ms] Published: TVOC=3883.4ppb | Actual=4.61 | Predict=4.60
[933338 ms] [USB CDC TEST] write failed (1)
[933342 ms] [sensor:68] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[933347 ms] [timer] LED2 toggles=270
[933350 ms] [SensorSim_Read OK]
[933354 ms] [IAQ_Predict OK]
[933356 ms] Published: TVOC=4112.2ppb | Actual=4.67 | Predict=4.66
[946973 ms] [USB CDC TEST] write failed (1)
[946977 ms] [sensor:69] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[946982 ms] [SensorSim_Read OK]
[946986 ms] [IAQ_Predict OK]
[946988 ms] Published: TVOC=4406.4ppb | Actual=4.74 | Predict=4.74
[960608 ms] [USB CDC TEST] write failed (1)
[960612 ms] [sensor:70] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[960617 ms] [SensorSim_Read OK]
[960621 ms] [IAQ_Predict OK]
[960623 ms] Published: TVOC=4550.2ppb | Actual=4.78 | Predict=4.78
[961001 ms] [timer] LED2 toggles=280
[974243 ms] [USB CDC TEST] write failed (1)
[974247 ms] [sensor:71] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[974252 ms] [SensorSim_Read OK]
[974256 ms] [IAQ_Predict OK]
[974258 ms] Published: TVOC=4773.8ppb | Actual=4.84 | Predict=4.85
[987878 ms] [USB CDC TEST] write failed (1)
[987882 ms] [sensor:72] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[987887 ms] [SensorSim_Read OK]
[987891 ms] [IAQ_Predict OK]
[987893 ms] Published: TVOC=4926.5ppb | Actual=4.88 | Predict=4.89
[998481 ms] [USB CDC TEST] write failed (1)
[998485 ms] [sensor:73] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[998491 ms] [timer] LED2 toggles=290
[998494 ms] [SensorSim_Read OK]
[998497 ms] [IAQ_Predict OK]
[998500 ms] Published: TVOC=5094.8ppb | Actual=5.00 | Predict=4.93
[1009084 ms] [USB CDC TEST] write failed (1)
[1009088 ms] [sensor:74] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1009094 ms] [SensorSim_Read OK]
[1009097 ms] [IAQ_Predict OK]
[1009100 ms] Published: TVOC=5193.4ppb | Actual=5.00 | Predict=4.96
[1019687 ms] [USB CDC TEST] write failed (1)
[1019691 ms] [sensor:75] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1019697 ms] [SensorSim_Read OK]
[1019700 ms] [IAQ_Predict OK]
[1019703 ms] Published: TVOC=5297.1ppb | Actual=5.00 | Predict=4.98
[1020001 ms] [timer] LED2 toggles=300
[1030290 ms] [USB CDC TEST] write failed (1)
[1030294 ms] [sensor:76] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1030300 ms] [SensorSim_Read OK]
[1030303 ms] [IAQ_Predict OK]
[1030306 ms] Published: TVOC=5412.7ppb | Actual=5.00 | Predict=4.99
[1040893 ms] [USB CDC TEST] write failed (1)
[1040897 ms] [sensor:77] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1040903 ms] [SensorSim_Read OK]
[1040906 ms] [IAQ_Predict OK]
[1040909 ms] Published: TVOC=5422.4ppb | Actual=5.00 | Predict=4.99
[1051496 ms] [USB CDC TEST] write failed (1)
[1051500 ms] [sensor:78] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1051506 ms] [timer] LED2 toggles=310
[1051509 ms] [SensorSim_Read OK]
[1051513 ms] [IAQ_Predict OK]
[1051516 ms] Published: TVOC=5536.8ppb | Actual=5.00 | Predict=5.00
[1062099 ms] [USB CDC TEST] write failed (1)
[1062103 ms] [sensor:79] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1062109 ms] [SensorSim_Read OK]
[1062112 ms] [IAQ_Predict OK]
[1062115 ms] Published: TVOC=5501.3ppb | Actual=5.00 | Predict=5.00
[1072702 ms] [USB CDC TEST] write failed (1)
[1072706 ms] [sensor:80] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1072712 ms] [SensorSim_Read OK]
[1072715 ms] [IAQ_Predict OK]
[1072718 ms] Published: TVOC=5456.0ppb | Actual=5.00 | Predict=5.00
[1073501 ms] [timer] LED2 toggles=320
[1083305 ms] [USB CDC TEST] write failed (1)
[1083309 ms] [sensor:81] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1083315 ms] [SensorSim_Read OK]
[1083318 ms] [IAQ_Predict OK]
[1083321 ms] Published: TVOC=5399.0ppb | Actual=5.00 | Predict=4.99
[1093908 ms] [USB CDC TEST] write failed (1)
[1093912 ms] [sensor:82] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1093918 ms] [SensorSim_Read OK]
[1093921 ms] [IAQ_Predict OK]
[1093924 ms] Published: TVOC=5290.9ppb | Actual=5.00 | Predict=4.98
[1104511 ms] [USB CDC TEST] write failed (1)
[1104515 ms] [sensor:83] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1104521 ms] [timer] LED2 toggles=330
[1104524 ms] [SensorSim_Read OK]
[1104528 ms] [IAQ_Predict OK]
[1104531 ms] Published: TVOC=5199.8ppb | Actual=5.00 | Predict=4.96
[1115114 ms] [USB CDC TEST] write failed (1)
[1115118 ms] [sensor:84] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1115124 ms] [SensorSim_Read OK]
[1115127 ms] [IAQ_Predict OK]
[1115130 ms] Published: TVOC=5085.7ppb | Actual=5.00 | Predict=4.93
[1125717 ms] [USB CDC TEST] write failed (1)
[1125721 ms] [sensor:85] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1125727 ms] [SensorSim_Read OK]
[1125730 ms] [IAQ_Predict OK]
[1125733 ms] Published: TVOC=4971.7ppb | Actual=4.89 | Predict=4.90
[1126501 ms] [timer] LED2 toggles=340
[1136320 ms] [USB CDC TEST] write failed (1)
[1136324 ms] [sensor:86] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1136330 ms] [SensorSim_Read OK]
[1136333 ms] [IAQ_Predict OK]
[1136336 ms] Published: TVOC=4830.2ppb | Actual=4.85 | Predict=4.86
[1146923 ms] [USB CDC TEST] write failed (1)
[1146927 ms] [sensor:87] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1146933 ms] [SensorSim_Read OK]
[1146936 ms] [IAQ_Predict OK]
[1146939 ms] Published: TVOC=4629.2ppb | Actual=4.80 | Predict=4.81
[1157526 ms] [USB CDC TEST] write failed (1)
[1157530 ms] [sensor:88] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1157536 ms] [timer] LED2 toggles=350
[1157539 ms] [SensorSim_Read OK]
[1157543 ms] [IAQ_Predict OK]
[1157546 ms] Published: TVOC=4417.1ppb | Actual=4.75 | Predict=4.75
[1168129 ms] [USB CDC TEST] write failed (1)
[1168133 ms] [sensor:89] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1168139 ms] [SensorSim_Read OK]
[1168142 ms] [IAQ_Predict OK]
[1168145 ms] Published: TVOC=4186.0ppb | Actual=4.69 | Predict=4.68
[1178732 ms] [USB CDC TEST] write failed (1)
[1178736 ms] [sensor:90] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1178742 ms] [SensorSim_Read OK]
[1178745 ms] [IAQ_Predict OK]
[1178748 ms] Published: TVOC=3913.5ppb | Actual=4.62 | Predict=4.61
[1179501 ms] [timer] LED2 toggles=360
[1189335 ms] [USB CDC TEST] write failed (1)
[1189339 ms] [sensor:91] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1189345 ms] [SensorSim_Read OK]
[1189348 ms] [IAQ_Predict OK]
[1189351 ms] Published: TVOC=3664.9ppb | Actual=4.55 | Predict=4.56
[1199938 ms] [USB CDC TEST] write failed (1)
[1199942 ms] [sensor:92] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1199948 ms] [SensorSim_Read OK]
[1199951 ms] [IAQ_Predict OK]
[1199954 ms] Published: TVOC=3400.7ppb | Actual=4.48 | Predict=4.48
[1210541 ms] [USB CDC TEST] write failed (1)
[1210545 ms] [sensor:93] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1210551 ms] [timer] LED2 toggles=370
[1210554 ms] [SensorSim_Read OK]
[1210558 ms] [IAQ_Predict OK]
[1210561 ms] Published: TVOC=3106.9ppb | Actual=4.41 | Predict=4.41
[1221144 ms] [USB CDC TEST] write failed (1)
[1221148 ms] [sensor:94] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1221154 ms] [SensorSim_Read OK]
[1221157 ms] [IAQ_Predict OK]
[1221160 ms] Published: TVOC=2913.1ppb | Actual=4.36 | Predict=4.36
[1231747 ms] [USB CDC TEST] write failed (1)
[1231751 ms] [sensor:95] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1231757 ms] [SensorSim_Read OK]
[1231760 ms] [IAQ_Predict OK]
[1231763 ms] Published: TVOC=2597.0ppb | Actual=4.28 | Predict=4.27
[1232501 ms] [timer] LED2 toggles=380
[1242350 ms] [USB CDC TEST] write failed (1)
[1242354 ms] [sensor:96] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1242360 ms] [SensorSim_Read OK]
[1242363 ms] [IAQ_Predict OK]
[1242366 ms] Published: TVOC=2280.7ppb | Actual=4.20 | Predict=4.19
[1252953 ms] [USB CDC TEST] write failed (1)
[1252957 ms] [sensor:97] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1252963 ms] [SensorSim_Read OK]
[1252966 ms] [IAQ_Predict OK]
[1252969 ms] Published: TVOC=2099.1ppb | Actual=4.15 | Predict=4.15
[1263556 ms] [USB CDC TEST] write failed (1)
[1263560 ms] [sensor:98] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1263566 ms] [timer] LED2 toggles=390
[1263569 ms] [SensorSim_Read OK]
[1263573 ms] [IAQ_Predict OK]
[1263576 ms] Published: TVOC=1854.1ppb | Actual=4.09 | Predict=4.08
[1274159 ms] [USB CDC TEST] write failed (1)
[1274163 ms] [sensor:99] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1274169 ms] [SensorSim_Read OK]
[1274172 ms] [IAQ_Predict OK]
[1274175 ms] Published: TVOC=1566.3ppb | Actual=4.01 | Predict=3.95
[1284762 ms] [USB CDC TEST] write failed (1)
[1284766 ms] [sensor:100] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1284772 ms] [SensorSim_Read OK]
[1284776 ms] [IAQ_Predict OK]
[1284778 ms] Published: TVOC=1367.4ppb | Actual=3.78 | Predict=3.76
[1285501 ms] [timer] LED2 toggles=400
[1295365 ms] [USB CDC TEST] write failed (1)
[1295369 ms] [sensor:101] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1295375 ms] [SensorSim_Read OK]
[1295379 ms] [IAQ_Predict OK]
[1295381 ms] Published: TVOC=1070.6ppb | Actual=3.51 | Predict=3.49
[1305968 ms] [USB CDC TEST] write failed (1)
[1305972 ms] [sensor:102] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1305978 ms] [SensorSim_Read OK]
[1305982 ms] [IAQ_Predict OK]
[1305984 ms] Published: TVOC=868.9ppb | Actual=3.33 | Predict=3.31
[1316571 ms] [USB CDC TEST] write failed (1)
[1316575 ms] [sensor:103] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1316581 ms] [timer] LED2 toggles=410
[1316584 ms] [SensorSim_Read OK]
[1316588 ms] [IAQ_Predict OK]
[1316591 ms] Published: TVOC=766.9ppb | Actual=3.24 | Predict=3.22
[1327174 ms] [USB CDC TEST] write failed (1)
[1327178 ms] [sensor:104] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1327184 ms] [SensorSim_Read OK]
[1327188 ms] [IAQ_Predict OK]
[1327190 ms] Published: TVOC=524.1ppb | Actual=3.02 | Predict=2.99
[1337777 ms] [USB CDC TEST] write failed (1)
[1337781 ms] [sensor:105] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1337787 ms] [SensorSim_Read OK]
[1337791 ms] [IAQ_Predict OK]
[1337793 ms] Published: TVOC=385.1ppb | Actual=2.60 | Predict=2.56
[1338501 ms] [timer] LED2 toggles=420
[1348380 ms] [USB CDC TEST] write failed (1)
[1348384 ms] [sensor:106] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1348390 ms] [SensorSim_Read OK]
[1348394 ms] [IAQ_Predict OK]
[1348396 ms] Published: TVOC=269.9ppb | Actual=2.30 | Predict=2.20
[1358983 ms] [USB CDC TEST] write failed (1)
[1358987 ms] [sensor:107] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1358993 ms] [SensorSim_Read OK]
[1358996 ms] [IAQ_Predict OK]
[1358999 ms] Published: TVOC=217.7ppb | Actual=2.17 | Predict=2.03
[1369586 ms] [USB CDC TEST] write failed (1)
[1369590 ms] [sensor:108] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1369596 ms] [timer] LED2 toggles=430
[1369599 ms] [SensorSim_Read OK]
[1369603 ms] [IAQ_Predict OK]
[1369606 ms] Published: TVOC=176.5ppb | Actual=2.06 | Predict=1.90
[1380189 ms] [USB CDC TEST] write failed (1)
[1380193 ms] [sensor:109] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1380199 ms] [SensorSim_Read OK]
[1380203 ms] [IAQ_Predict OK]
[1380205 ms] Published: TVOC=144.0ppb | Actual=1.86 | Predict=1.80
[1390792 ms] [USB CDC TEST] write failed (1)
[1390796 ms] [sensor:110] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1390802 ms] [SensorSim_Read OK]
[1390806 ms] [IAQ_Predict OK]
[1390808 ms] Published: TVOC=144.8ppb | Actual=1.86 | Predict=1.80
[1391501 ms] [timer] LED2 toggles=440
[1401395 ms] [USB CDC TEST] write failed (1)
[1401399 ms] [sensor:111] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1401405 ms] [SensorSim_Read OK]
[1401409 ms] [IAQ_Predict OK]
[1401411 ms] Published: TVOC=108.2ppb | Actual=1.64 | Predict=1.69
[1411998 ms] [USB CDC TEST] write failed (1)
[1412002 ms] [sensor:112] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1412008 ms] [SensorSim_Read OK]
[1412012 ms] [IAQ_Predict OK]
[1412014 ms] Published: TVOC=145.3ppb | Actual=1.87 | Predict=1.80
[1422601 ms] [USB CDC TEST] write failed (1)
[1422605 ms] [sensor:113] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1422611 ms] [timer] LED2 toggles=450
[1422614 ms] [SensorSim_Read OK]
[1422618 ms] [IAQ_Predict OK]
[1422621 ms] Published: TVOC=245.5ppb | Actual=2.24 | Predict=2.12
[1433204 ms] [USB CDC TEST] write failed (1)
[1433208 ms] [sensor:114] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1433214 ms] [SensorSim_Read OK]
[1433218 ms] [IAQ_Predict OK]
[1433220 ms] Published: TVOC=277.9ppb | Actual=2.32 | Predict=2.22
[1443807 ms] [USB CDC TEST] write failed (1)
[1443811 ms] [sensor:115] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1443817 ms] [SensorSim_Read OK]
[1443821 ms] [IAQ_Predict OK]
[1443823 ms] Published: TVOC=420.1ppb | Actual=2.69 | Predict=2.67
[1444501 ms] [timer] LED2 toggles=460
[1458218 ms] [USB CDC TEST] write failed (1)
[1458222 ms] [sensor:116] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1458228 ms] [SensorSim_Read OK]
[1458232 ms] [IAQ_Predict OK]
[1458235 ms] Published: TVOC=606.8ppb | Actual=3.09 | Predict=3.08
[1471853 ms] [USB CDC TEST] write failed (1)
[1471857 ms] [sensor:117] AHT20 err=1 (1=NACK 2=BUSY 3=TIMEOUT)
[1471862 ms] [SensorSim_Read OK]
[1471866 ms] [IAQ_Predict OK]
[1471869 ms] Published: TVOC=721.0ppb | Actual=3.19 | Predict=3.18