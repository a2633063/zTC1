# zTC1
**斐讯TC1智能排插个人固件.**

斐讯0元购完全翻车了,服务器也已经关闭.智能排插TC1也因为服务器关闭,app无法登陆,定时开关等功能完全无法使用.

为此,针对TC1智能排插的硬件,开发供自己使用的FW及对应app,确保自己能够正常使用此排插.取名为zTC1.



APP端见: [SmartControl_Android_MQTT](https://github.com/a2633063/SmartControl_Android_MQTT)



**本文档只做简单介绍,如果你要使用,可以从[点这里](https://github.com/a2633063/zTC1/wiki)开始**

本项目还处于开发前期,可能会有bug,文档不全等缺点!敬请注意


## 编译烧录

### 编译

> 如果你不是开发人员,可以忽略此项

TC1使用的主控为EMW3031,基于MiCO(MCU based Internet Connectivity Operating System)开发.[MiCO简介点这里](http://developer.mxchip.com/handbooks/101)

需要按照官方说明才能保证此项目能够编译成功:

1. 安装[MiCO Cube编译工具](http://developer.mxchip.com/handbooks/102)
2. 配置[MICoder IDE环境](http://developer.mxchip.com/handbooks/105)
3. 配置[Jlink下载工具](http://developer.mxchip.com/handbooks/103)
4. check out 此项目,按照[从一个现有的 Git 仓库克隆导入](http://developer.mxchip.com/handbooks/102#从一个现有的-git-仓库克隆导入)确认项目编译/下载正常

### 烧录

[zTC1-固件烧录](https://github.com/a2633063/zTC1/wiki/zTC1-固件烧录)



### 使用

[开始使用](https://github.com/a2633063/zTC1/wiki/开始使用)

## 功能

暂时计划完成以下功能,后续有需要继续添加

- [x] wifi (断连)自动连接

- [x] app 直接配置wifi连接

- [x] SNTP网络校时
- [x] MQTT客户端连接服务器
- [x] app控制每个接口单独开关(测试版本)
- [x] 配置每个接口单独定时开关(未测试验证)
- [x] 配置每个接口单独倒计时开关
- [ ] app实时显示功率
- [ ] ~~根据功率自动开关~~



根据以上功能,此项目针对EMW3031,需要完成以下功能


- [x] 掉电保存数据

- [x] wifi自动连接

- [x] Easylink

- [x] SNTP自动校时

- [x] MQTT客户端连接服务器,订阅/发布主题

- [x] Json 数据处理(后续继续更新)

- [x] MQTT设置各项配置

- [x] 单独提供一个UDP/TCP端口供配置

  
