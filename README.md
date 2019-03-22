# zTC1
**斐讯TC1智能排插个人固件.**

斐讯0元购完全翻车了,服务器也已经关闭.智能排插TC1也因为服务器关闭,app无法登陆,定时开关等功能完全无法使用.

为此,针对TC1智能排插的硬件,开发供自己使用的FW及对应app,确保自己能够正常使用此排插.取名为zTC1.



APP端见: [SmartControl_Android_MQTT](https://github.com/a2633063/SmartControl_Android_MQTT)

> 如果你有任何问题,可以直接在此项目中提交issue,或给我发送邮件:zip_zhang@foxmail.com,邮件标题中请注明项目



## 直接为使用看这里

如果你只是为了TC1能够继续使用不做技术开发交流等,请直接查看[使用步骤及方法](https://github.com/a2633063/zTC1/wiki#使用步骤及方法), 





![图片来源斐讯官网产品介绍](https://raw.githubusercontent.com/wiki/a2633063/zTC1/image/Phicomm_TC1.png)



## [单击从这里开始](https://github.com/a2633063/zTC1/wiki)





## 功能


- [x] 暂时计划完成以下功能,后续有需要继续添加

- [x] 4个USB充电(3个普通和1个快充接口)(USB充电软件无法控制,所有应该和原固件功能相同)
- [x] MQTT客户端连接服务器(无MQTT时使用UDP通信,如果你会内网穿透,就可能实现远程控制功能)
- [x] app控制每个接口独立开关
- [x] app配置每个接口拥有独立的5组定时开关
- [ ] ota在线升级(暂时未做)
- [ ] app实时显示功率(暂时未测量功率模块的电路.暂时未实现此功能)
- [ ] ~~根据功率自动开关~~(可能不会做这个功能)

  

