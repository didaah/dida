<?php
// $Id$

/**
 * @file
 *  用户个人中心，模板加载顺序：
 *    user_center_{$center->type}.tpl.php
 *    user_center.tpl.php
 * @param object $center
 *  个人中心对象
 *    array $center->menu - 功能菜单注册信息
 *    string $center->links - 经过处理的功能菜单
 *    string $center->type - 当前页面标识
 *    string $center->path - 当前页面路径
 *    string $center->title - 当前页面名称
 *    array $center->tabs - 选项标签
 *    string $center->body - 当前页面内容
 *    其它模块自定义数据
 *  
 */

global $user;

?>

<div id="user-center" class="user-center-content user-center-content-<?php echo $center->type;?>">

  <div id="user-center-link">
    <?php echo $center->links?>
  </div>

  <div id="user-center-body">
    <?php echo theme('item_list', $center->tabs, NULL, 'ul', array('class' => 'nav nav-tabs tabs user-center-tabs'), 0); ?>
    <?php echo $center->body;?>
  </div>

</div>
