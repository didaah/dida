<?php
// $Id$

/**
 * 模块定义的所有 hook
 */
function user_hook_info() {
  $hook = array();
   
  $hook['user_center_info'] = array(
    'name' => '个人中心页面注册',
    'description' => '向用户个人中心注册页面，同时根据注册信息生成 menu',
    'return' => array( // 返回数据
      'type' => 'array', // 返回数据类型
      'data' => array( // 返回数据示例
        // pass 为页面标识，若重复定义，根据模块加载顺序覆盖
        'newpass' => array(
          'title' => '修改密码', // 必须，页面标题
            /**
             * path 可选，页面路径，若不设置，默认使用标识加个人中心前缀，即 user/center/newpass
             * 注意：默认情况下，不需要在 hook_menu 中定义 user/center/newpass
             */
//          'path' => '', 
//          'access_callback' => '', // 可选，权限验证函数，默认登录用户即可使用
//          'access_arguments' => array(''), // 可选，权限验证参数
//          'weight' => 100, // 可选，页面权重，数值越小，排名越前
//          'group' => 'user', // 可选，页面所属模块组，默认为本模块
//          'attributes' => array(), // 可选，链接参数，即 l() 函数的 attributes 部分
        ) 
      ),
    )
  );
    
  $hook['user_center_view'] = array(
    'name' => '个人中心页面显示',
    'description' => '',
    'arguments' => array( // 传递的参数
      'center' => array(
        'type' => 'object',
        'name' => '页面对象',
        'description' => '该对象将传递给个人中心模板文件，可为对象 $center->body 赋值。通过 $center->type 判断当前页面'
      ),
    ),
  );
     
  $hook['user_page_info'] = array(
    'name' => '用户扩展页面定义',
    'description' => '',
    'arguments' => array(
      'account' => array(
        'type' => 'object',
        'name' => '用户对象',
        'description' => '当前用户对象'
      ),
    ), 
    'return' => array(
      'type' => 'array',
      'description' => '返回一个定义页面类型的数组，一个模块可以定义多个页面'
    )
  );
     
  $hook['user_page_view'] = array(
    'name' => '用户扩展页面显示',
    'description' => '',
    'arguments' => array( // 传递的参数
      'account' => array(
        'type' => 'object',
        'name' => '用户对象',
        'description' => '当前用户对象，可为对象 $account->content 赋值，约定输出该内容'
      ),
      'type' => array(
        'type' => 'string',
        'name' => '页面类型',
        'description' => '模块可根据该类型判断是否需要输出或改写 $account 对象'
      ),
    ),
    'return' => array(), // 无返回，使用引用传递，直接修改 $account 对象
  );
    
  return $hook;
}


