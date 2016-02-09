<?php
// $Id$

/**
 * 如果你需要更灵活的多站点配置，请将本文复制一份，更名为 config.php
 * 然后，请详细阅读以下说明：
 *
 * config.php 用来设置站点配置目录
 * 通常情况下，配置在 sites/ 目录下
 * 而多站点，以当前访问域名作为判断，读取类似目录 sites/example.com
 * 这并不是一种灵活的方式，当需要改变域名时，必须重命名目录
 *
 * config.php 提供了一种更灵活的设置，你可以自定义配置目录
 * 原则上，该目录应该在 sites/，这可以让程序结构更清晰
 * 当然，这并非强制，只要 php 有权限操作，都可以做为配置目录
 * 比如：当通过 www.myname.com 访问时，你可以指定 sites/host 做为配置目录
 * 若未指定，将寻找 sites/www.myname.com
 * 亦即是，在此处的设置，优先级是最高的
 * 设置格式为一个数组，域名为键，配置目录为值
 * 
 * default：该键名为默认值，假如设置了该单元，则所有未指定配置的域名访问，都使用它
 *
 * 即使不需要利用同一套代码创建多个站点，也建议使用这种方式进行配置安装，以便于日后扩展
 */
/**
 * 示例
 */
$config = array(
  'default' => 'sites/default', // 默认，所有未指定的，使用此配置。可不设置
  'www.example.com' => 'sites/all/myhost',
  'photo.example.com' => 'sites/all/photo',
  'www.myname.com' => 'sites/all/photo'
);
