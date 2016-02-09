<?php
// $Id$

/**
 *  template.php 能够让主题变得更灵活、更强大，通过它，可以对模块输出的内容进行的重写，以利于制作出更优良的主题
 * 这是 template.php 文件的示例，若需使用，请更名为 template.php
 *
 * @file 
 *  可选，若文件(template.php)存在，则系统初始化时将加载。
 *  可在文件中创建主题配置、定义方法、实现 hook 等。
 */

/**
 * @Implement of hook_settings_form()
 * 主题设置表单，用于主题保存自定义数据，有利于制作更灵活的主题
 * 若字段定义在 settings 组之下，所有内容将自动保存到 $conf['settings'] 内
 */
function default_settings_form(&$form, $conf) {
  
  $form['fields']['settings']['theme_test'] = array(
    '#title' => '自定义字段',
    '#default_value' => $conf['settings']['theme_test'],
    '#type' => 'textfield',
    '#description' => '该字段自动保存'
  );

  $form['fields']['theme_test2'] = array(
    '#title' => '测试',
    '#type' => 'textfield',
    '#description' => '该字段不会自动保存，请自行处理'
  );

  /**
   * 在主题任何地方可以此方法获取以上自定义字段：
   * global $conf;
   * echo $conf['themes']['default']['settings']['theme_test'];
   * // default 为主题名
   * // 未自动保存的字段，无法获取，应自行处理
   */
}

/**
 * @Implement of hook_alter_css()
 * 可对页面 css 文件进行增减
 */
function default_alter_css(&$css) {

  /**
   * 去除由 system 模块加载的 system.css 文件
   */
  unset($css['files'][dd_get_path('module', 'system') . '/system.css']);

  /**
   * 增加一个 css，键名即为 css 文件路径，相对于程序根目录。
   */
  $css['files'][dd_get_path('theme', 'default') . '/test.css'] = array('type' => 'theme', 'cached' => true);
}

/**
 * @Implement of hook_alter_js()
 * 可对页面 js 文件进行增减
 */
function default_alter_js(&$js) {

  /**
   * 去除由 system 模块加载的 system.js 文件
   */
  unset($js['files'][dd_get_path('module', 'system') . '/system.js']);

  /**
   * 增加一个 js，键名即为 js 文件路径，相对于程序根目录。
   */
  $js['files'][dd_get_path('theme', 'default') . '/test.js'] = array('type' => 'theme', 'cached' => true);
}

/**
 * @Implement of hook_template_x()
 * 覆写一个 theme 输出，其它 hook_theme() 类同
 */
function default_template_breadcrumb($breadcrumb) {
  return '<div class="breadcrumb theme_test">' . implode(' › ', $breadcrumb). '</div>';
}


