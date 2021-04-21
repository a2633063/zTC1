# zTC1 a1版
**斐讯TC1智能排插个人固件.**

github无法打开查看的可在gitee查看:https://gitee.com/a2633063/zTC1

排插TC1因为服务器关闭,无法使用.

为此,开发供自己使用的FW及对应app,确保自己能够正常使用此排插.取名为zTC1.

>  注意:
>
>  ​	TC1排插硬件分a1 a2两个版本,本固件仅支持**a1版本.**
>
>  ​	a1 a2两个版本仅主控不同,除此之外其他无任何区别
>
>  ​	区分硬件版本见[区分硬件版本](#区分硬件版本)

建立了个QQ群,有问题可以加入来讨论:**459996006**  [点这里直接加群](//shang.qq.com/wpa/qunwpa?idkey=9104eabd6131d856b527ad89636fc603eb745a5d047e8b45d183165c8e607e59)  备注 zTC1
群内有免费的mqtt服务器分享,实现远程控制功能!

也可以发送邮件给我:zip_zhang@foxmail.com

注意:所有咨询我文档已经写清楚的问题等等,都不再做任何回复

注意: 固件增加了激活码的功能,所以提供完整固件包
已经发现有人使用的我的固件在我未授权的情况下在pdd转转等上加价售卖,~~所以之后不再提供完整刷机包,仅提供ota包,需要完整包刷机的请加群索要,或是发送邮件给我并能够证明你不是店铺卖家也会单独提供给你完整固件~~

> ### 作者声明
>
> 注意: 本项目主要目的为作者本人自己学习及使用TC1排插而开发,本着开源精神及造福网友而开源,仅个人开发,可能无法做到完整的测试,所以不承担他人使用本项目照成的所有后果.
>
> **严禁他人将本项目用户用于任何商业活动.个人在非盈利情况下可以自己使用,严禁收费代刷等任何盈利服务.**
>
> 有需要请联系作者:zip_zhang@foxmail.com



~~不定时重启断电!!!~~  已修复重启问题(0.10.3),此版本开始需要使用激活码才能使用,激活码原则上免费提供给个人使用.详细见[激活码](#激活码说明)









## 特性

本固件使用斐讯TC1排插硬件为基础,实现以下功能:

- [x] 4个USB充电(3个普通和1个快充接口)(硬件直接实现,与软件无关)
- [x] 按键控制所有插口通断
- [x] 控制每个接口独立开关
- [x] 每个接口拥有独立的5组定时开关
- [x] ota在线升级
- [x] 无服务器时使用UDP通信
- [x] MQTT服务器连接控制
- [x] 通过mqtt连入homeassistant
- [x] app实时显示功率
- [ ] ~~根据功率自动开关~~(未做此功能)

后续计划及当期正在处理的见[TodoProject](https://github.com/a2633063/zTC1/projects/1)



<img src="https://cdn.jsdelivr.net/gh/a2633063/Image/zTC1/Phicomm_TC1.png" width="540">





## 目录

[前言(必看)](#前言必看)

[区分硬件版本](#区分硬件版本)

[开始](#开始)

[激活码说明](#激活码说明)

[拆机接线及烧录固件相关](#拆机接线及烧录固件相关)

[开始使用/使用方法](#开始使用/使用方法)

[接入home assistant](#接入home-assistant)

[其他内容](#其他内容)

​	[代码编译](#代码编译)

​	[通信协议](#通信协议)

​	[FAQ](#FAQ)

[文档更新日志](#文档更新日志)





## 前言(必看)

- 除非写明了`如果你不是开发人员,请忽略此项`之类的字眼,否则,请**一个字一个字看清楚看完**整后再考虑动手及提问!很可能一句话就是你成功与否的关键!
- 烧录固件需要专用的烧录器:支持swd的jlink烧录器,目前已知便宜的价格为不到20元包邮.(本人不做烧录器的售卖,所有提供的链接或推荐都为第三方卖家,和本人无关).
- 使用此固件,需要app端配合,见[SmartControl_Android_MQTT](https://github.com/a2633063/SmartControl_Android_MQTT).
- app只有android,因ios限制,本人不考虑免费做ios开发.(不要再问是否有ios端).

> 虽然没有ios端,但固件支持homeassistant,可以使用安卓APP配置完成后,连入homeassistant后,使用ios控制. APP主要仅为第一次使用配对网络及配置mqtt服务器时使用,之后可以用homeassistant控制不再使用app.

> 如果你不知道什么是mqtt或homeassistant,所有有关的内容可以跳过.

> 如果你有任何问题,可以直接在此项目中提交issue,或给我发送邮件:zip_zhang@foxmail.com,邮件标题中请注明[zTC1].
>
> 



## 区分硬件版本

硬件版本在外包装底部,如图所示:

![hardware_version](https://cdn.jsdelivr.net/gh/a2633063/Image/zTC1/hardware_version.png)

如果没有包装,只能[拆开](#拆机接线及烧录固件相关)分辨,如图,左侧为不支持的a2版本,右侧为支持的a1版本



![a1_a2](https://cdn.jsdelivr.net/gh/a2633063/Image/zTC1/a1_a2.png)

## 开始

整体流程如下:拆开TC1,将固件/烧录器/pc互相连接,在pc运行烧录软件进行烧录,烧录固件.

烧录完成后,首次使用前配对网络并配置mqtt服务器,之后就可以使用了.



## 激活码说明

为防止被不良商用,增加了必须使用激活码激活的功能.对于一般个人申请激活码限额免费获取

见[自助获取激活码](https://github.com/a2633063/SmartControl_Android_MQTT/wiki/激活码获取)

激活方式见[开始使用中的激活](https://github.com/a2633063/zTC1/wiki/开始使用#激活)

## 拆机接线及烧录固件相关

见[固件烧录](https://github.com/a2633063/zTC1/wiki/固件烧录)

烧录固件完成后,即可开始使用



## 开始使用/使用方法

见[开始使用](https://github.com/a2633063/zTC1/wiki/开始使用)



## 接入home assistant

见[homeassistant接入](https://github.com/a2633063/zTC1/wiki/homeassistant接入)



## 其他内容

### 代码编译

> 此项为专业开发人员准备,如果你不是开发人员,请跳过此项

TC1使用的主控为EMW3031,基于MiCO(MCU based Internet Connectivity Operating System)开发.[MiCO简介点这里](http://developer.mxchip.com/handbooks/101)

需要按照官方说明才能保证此项目能够编译成功:

1. 安装[MiCO Cube编译工具](http://developer.mxchip.com/handbooks/102)
2. 配置[MICoder IDE环境](http://developer.mxchip.com/handbooks/105)
3. 配置[Jlink下载工具](http://developer.mxchip.com/handbooks/103)
4. check out 此项目,按照[从一个现有的 Git 仓库克隆导入](http://developer.mxchip.com/handbooks/102#%E4%BB%8E%E4%B8%80%E4%B8%AA%E7%8E%B0%E6%9C%89%E7%9A%84-git-%E4%BB%93%E5%BA%93%E5%85%8B%E9%9A%86%E5%AF%BC%E5%85%A5)确认项目编译/下载正常



### 通信协议

> 此项为专业开发人员准备,如果你不是开发人员,请跳过此项

所有通信协议开源,你可以自己开发控制app或ios端

见[通信协议](https://github.com/a2633063/zTC1/wiki/通信协议)



### FAQ

见 [FAQ](https://github.com/a2633063/SmartControl_Android_MQTT/wiki/FAQ)



### 文档更新日志

**20200506**

修改文档图库地址

**20191224**

增加FAQ跳转链接

**2019年10月10日**

更新文档适用于v1.0.2版本固件:通信协议、homeassistant接入

**2019年9月23日17:18:10**

更新文档适用于v1.0.0版本固件:开始使用、通信协议、homeassistant接入
