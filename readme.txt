list:
 getkey.c       //按键调试程序源码
 keypad.c       //矩阵键盘内核驱动源码
 keypad-hw.h    //矩阵键盘内涵驱动源码
 Makefile       //编译脚本

 编译命令:
 make           //编译矩阵键盘内核驱动模块,等到rha-keypad.ko
 make app       //编译按键调试程序,等到getkey
 make clean     //清理工程

使用方法：
insmod rha-keypad.ko        //插入驱动模块
rmmod rha-keypad            //卸载驱动模块
./getkey /dev/eventX        //获取按键信息



