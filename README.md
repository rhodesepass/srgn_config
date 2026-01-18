# srgn_config

电子通行证 配置工具

这是一个类似于 raspi-config 的配置工具，用于设置和管理电子通行证底层（Device Tree相关）参数。

## 配置原理

程序修改/boot/uEnv.txt. uEnv.txt的内容为：

```
bootargs=console=ttyS0,115200 console=tty0 panic=5 rootwait mtdparts=spi0.0:1M(u-boot)ro,-(rootfs) root=ubi0:rootfs earlyprintk rw rootfstype=ubifs ubi.mtd=1
kernelfn=zImage
interface=usbhs i2c0
ext=cardkb
```

interface 就是配置的接口，按空格分开

ext就是拓展功能 按空格分开。

## 配置内容

需要处理引脚冲突问题。

0.4及以下的引出引脚为：PE11 PE5 PE6 PE7 PE8 PE9 PE10 PA1 PA2 PA3 PD0 PD12

0.5及以上的引出引脚为：PE3 PE5 PE6 PE7 PE8 PE9 PE10 PA1 PA2 PA3 PD0 PD12

### 接口

* adc_pa123.dts :将PA1/PA2/PA3全部用作ADC引脚
* adc_pa1.dts ：将PA1用作ADC引脚
* i2c0.dts：启动I2C，并将PD0 PD12作为I2C引脚
* i2s0_pa.dts 启动I2S，并将PA1/PA2/PA3/PE3用作I2S引脚。
  * 0.5及以上才可使用
  * 与i2s_pe.dts互斥
* i2s0_pe.dts 启动I2S，并将PA1/PE5/PE6/PE3用作I2S引脚。
  * 0.5及以上才可使用
  * 与i2s_pa.dts互斥
* spi1.dts 启动SPI1，并将PE7/PE8/PE9/PE10用作SPI引脚。
* uart1.dts 启动UART1，并将PA2/PA3用作UART1引脚
* uart2.dts 启动UART2，并将PA7/PA8用作UART2引脚
* usbhost.dts 将USB模式设置为USB Host
* usbhs.dts 启动USB2.0 High-Speed模式

### 拓展

* cardkb.dts 支持M5Stack CardKB。需要使能i2c0
* lsm6ds3_pre0.4.dts 支持0.4及以前的板载IMU，需要使能i2c0，0.4及以上才可使用


## 开源感谢：

* AnbUI[https://github.com/oerg866/anbui](https://github.com/oerg866/anbui):(Very) Tiny Text UI library