// $Id$

自定义安装：
1、若 install/install.php 文件存在，且 install() 存在，将包含此文件，并调用 install() 来安装系统
2、若 install/install.custom.php 文件存在，且 install_custom() 存在，则从默认方式安装成功后，包含此文件，并调用 install_custom() 函数