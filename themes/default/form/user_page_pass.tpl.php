<?php
/**
 * @file 
 *  自定义表单模板文件，命名规则：{$form_id}.tpl.php
 *  当模板文件存在，不再输出默认 html，系统载入该文件，表单输出以该文件为准
 *  本文件为找回密码表单的模板文件，做为示例
 *
 * @param array $form
 *  表单原始数据
 * @param array $fields
 *  经过 dd_get_form() 解析的表单字段数据
 *
 * @param array $error
 *  表单提交后，若验证错误，返回的错误信息，对应表单字段
 * @param array $form_values
 *  表单提交值，即 $_POST
 * 
 * @param ....
 *  所有其它传递的参数，不同表单参数不同
 *
 * 自定义表单 html，只要求字段 name 一致，其它可自由调整
 * 但 id class 等的更改，可能导致默认的 js 失效，需要根据表单做具体处理
 *
 */

//var_dump($form);
//var_dump($fields);
//var_dump($error);

?>

<?php

/**
 * 示例一：
 *  最简单方式，直接调用系统函数输出 html
 *  这样可保持字段内部的 html 结构不变，js 等验证不会失效，但定制性不高
 */

echo dd_form_html($form, $fields);

?>

<?php 
  /**
   * 示例二：
   *  对每个字段单独输出，必须保证每个字段都输出，否则可能无法通过表单验证
   */

  //echo $fields['mail']['#html'];
  //echo $fields['name']['#html'];
  //echo $fields['submit']['#html'];
?>

<?php
  /**
   * 以下为示例三：完全自定义，包括错误信息
   * 表单字段默认值，可在 $fields 中获取，假如字段名为：title，则为 $fields['title']['#default_value']
   * 注意：
   *  字段的 name 属性必须一致
   *  必须在合适的位置输出 $error 信息，否则可能验证错误，但信息没有输出，导致用户不明白发生什么
   */
  if (1 == 0) : 
?>


<p>邮箱：<input type="text" value="<?php echo $fields['mail']['#default_value'] ?>" name="mail" /></p>
<p>昵称：<input type="text" value="<?php echo $fields['name']['#default_value'] ?>" name="name" /></p>
<p><input type="text" value="找回密码" name="submit" /></p>

<?php 
  // 判断错误信息
  if (!empty($error['mail']['#error'])) :
  /**
   *  错误信息为一个数组，可以使用 implode('', $error['mail']['#error']);
   *  也可以自行显示错误信息，如：
   */
  echo '错误，不能为空';
  endif;
?>

<?php endif; ?>
