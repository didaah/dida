<?php
// $Id$

/**
 * @file
 *  dida 采用用户输入内容加随机码的方式生成密码，本脚本提供重置指定用户密码的功能
 *  终端命令示例：
 *    // 将 www.cmsns.com 站点下昵称为 admin 的用户密码设置为 test
 *    php common.inc --host www.cmsns.com --bootstrap data --script_name pass.inc --name admin --pass test
 *    // 将 www.cmsns.com 站点下 uid 为 1 的用户密码设置为 test
 *    php common.inc --host www.cmsns.com --bootstrap data --script_name pass.inc --uid 1 --pass test
 */

if (!empty($_args['pass'])) {
  
  // 加载 user 模块主文件，需要使用 user_get_salt_pass() 生成密码
  require_once DIDA_ROOT . '/modules/user/user.module';
  $pass = user_get_salt_pass($_args['pass']);

  $ok = 0;

  if (!empty($_args['name'])) {
    $ok = db_exec('UPDATE {users} SET pass = ?, salt = ? WHERE name = ?',
    array($pass['pass'], $pass['salt'], $_args['name']));
  } else if (!empty($_args['uid'])) {
    $ok = db_exec('UPDATE {users} SET pass = ?, salt = ? WHERE uid = ?',
    array($pass['pass'], $pass['salt'], $_args['uid']));
  }

  echo $ok ? '修改成功 - success' : '修改失败 - failure';
  echo "\n";
  exit();
}
