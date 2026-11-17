# tinyFOC, a high efficient Field Oriented Control (FOC) algorithm in fix point.

## Source

+ src
|
|

## Compile

```
make
make CPU=Cortex-M0+
```

## Hardware

tinyFOC algorthim is validated on ST MCU and Puya MCU with
EmbedFire Motor development board and PMSM brushless motor.

F407骄阳电机开发板
https://github.com/Embedfire-motor/ebf_motor_tutorial_stm32f407

| MCU      | Main Frequency | Execution time per FOC loop |
|----------|----------------|-----------------------------|
| ST32F408 | 168MHz         | 4us                         |
| PY32F030 | 48MHz          | 55us                        |








Tipasig USB Protocol Analyzer相关的技术分析帖：

《USB协议分析仪使用集锦之一：AMI BIOS的锅》
https://bbs.eeworld.com.cn/thread-1371597-1-1.html

《USB协议分析仪使用集锦之二：ULPI时序控制问题》
https://bbs.eeworld.com.cn/thread-1372421-1-1.html

《USB协议分析仪使用集锦之三：克隆一个USB Sniffer》
https://bbs.eeworld.com.cn/thread-1356060-1-1.html
