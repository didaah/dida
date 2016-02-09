<?php
// $Id$

/**
 * @file 用户个人页扩展页面默认模板
 *
 * 可根据页面类型使用不同模板文件，模板文件加载顺序：
 *  user_page_$type.tpl.php
 *  user_page.tpl.php
 * 所有 user/$uid/page/* 下的页面，皆为扩展页面，由其它模块生成
 * @param object $account
 *  用户对象，其中约定  $account->content 为默认输出内容
 * @param string $type
 *  扩展页面类型，该变量等于 arg(3)
 */

?>

<div class="user_page" id="user_page_<?php echo $type; ?>">
  <?php echo $account->content; ?>
</div>
