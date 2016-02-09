<?php
// $Id$

/**
  $form = array();
  
  // 表单配置，设置验证函数等参数
  $form['settings'] = array(
    '#validate' => array('module_test_form_validate'),
  );

  // 设置字段分组 fieldset 信息 
  $form['groups'] = array('test' => array('#fieldset' => true, '#fieldset_sort' => 'desc'));

  // 表单字段
  $form['fields'] = array();

  $form['fields']['account'] = array(
    '#type' => 'textfield',
    '#title' => '用户名',
    '#required' => true,
    '#default_value' => $ac[0],
    '#prefix' => '包裹前缀',
    '#suffix' => '包裹后缀',
    '#field_prefix' => '字段前缀',
    '#field_suffix' => '字段后缀',
    '#groups' => 'test', // 字段分组，同组字段输出到页面时，将显示在一起。并可包裹在 fieldset 中，根据  $form['settings']['#groups'] 中的设置而定
    '#attributes' => array(
      'id' => 'user_login',
      'size' => 50,
      'class' => "{required:true, messages:{required:'请输入用户名'}}"
    ),
    '#autocomplete' => array(
      'url' => url('user/tttt'),
    )
  );
*/

/**
 * 调用一个表单
 * @param string $form_id 
 *  表单 id，即函数名称，系统通过此 id 回调表单定义函数，并且将表单缓存与此 id 关联。
 * @access public
 * @return string
 */
function dd_get_form($form_id) {
  global $conf, $user, $form_tokens, $base_theme_name;
  dd_add_js('misc/form.js');
  
  $args = func_get_args();
  array_shift($args);

  // 同一页面，同一表单，只能调用一次
  if ($_POST['form_id'] == $form_id) {
  	$form_token = $_POST['form_token'];
  } else {
  	$form_token = md5($user->uid . uniqid(mt_rand(), true));
  }
  
  $form_tokens[$form_id] = $form_token;

  // 调用定义表单的函数，获得表单原始数据
  $form = call_user_func_array($form_id, $args);
  
  if (!is_array($form['fields'])) {
    return false;
  }
  
  if ($form['#form_token']) {
    $form_token = $form['#form_token'];
  }
  
  $form['__args'] = $args;

  // 执行 hook_form_alter()，可以在此 hook 中实施覆写
	module_alter_all('form_alter', $form_id, $form_token, $form);
  
  $form['#args'] = array(
  	'method' => 'post',
  	'action' => "",
  	'id' => $form_id,
  	'accept-charset' => 'UTF-8',
  );

  // 表单属性
  if ($form['settings'] && is_array($form['settings']['#attributes'])) {
  	$form['#args'] = array_merge($form['#args'], $form['settings']['#attributes']);
  }
  
  $form['#form_id'] = $form_id;
  $form['#form_token'] = $form_token;

  // 表单提交成功后重定向路径
  if (!$form['settings']['#redirect'] && $form['#args']['#action']) {
  	$form['settings']['#redirect'] = $form['#args']['#action'];
  }
  
  $fields = $form['fields'];
  
  //如果表单请求ajax验证，则载入以下js。
  if ($form['settings']['#ajax_validate']) {
    dd_add_js('misc/jquery.validate.js');
    dd_add_js('misc/jquery.metadata.js');
    if (isset($form['settings']['#ajax_submit'])) {
      //如请求验证完成后ajax提交，则载入以下js。
      //ajax默认提交到当前url，可在表单中设置
      /*
        '#ajax_validate' => true,
        '#ajax_submit' => array(
          'des' => true,
          'reset' => true,
          'options' => array(
            'dataType' => 'script',
            'url' => url('user/register', array('query' => array('ajax' => 'submit'))),
          ),
        ),
      */
      dd_add_js('misc/jquery.form.js');
    }
    
    $ajax_js_value = array();
    
    if (is_array($form['settings']['#ajax_validate'])) {
      $ajax_js_value = $form['settings']['#ajax_validate'];
    }
    
    $ajax_js_value['form_id'] = $form_id;
    $ajax_js_value['ajax_submit'] = isset($form['settings']['#ajax_submit']);
    $ajax_js_value['options'] = $form['settings']['#ajax_submit']['options'];
    
    dd_add_js(array('ajax_validate' => array($ajax_js_value)), 'setting');
  }
  
  // 处理当前提交的表单
  if ($_POST['form_id'] == $form_id) {
    $form_values = &$_POST;

    $form_values['_dida_values'] = (object) $_POST;
    
  	// 基本验证：必填字段，选项字段数据是否被修改
    if ((!$error = dd_form_required($form, $form_values)) && (!$error = file_form_validate($form, false))) {
    	// 回调表单验证函数
    	
      if (is_array($form['settings']['#validate'])) {
        foreach ($form['settings']['#validate'] as $function) {
          if ($error = $function($form, $form_values)) {
            break;
          }
        }
      }
      
      if ($error && is_array($error)) {
        $error = array_filter($error);
      }
      
      // 所有验证通过后再处理文件字段
      if (!$error && (!$error = file_form_validate($form, true))) {
      	// 文件字段处理完毕后回调提交函数
      	
        //删除表单session，防止重复提交
        dd_del_form_cache($user->uid, $form_id, $form_token);
        
        //回调提交函数，处理数据。
        form_save_submit($form, $form_values);
        
        //重定向页面
        
        session_write_close();
        
        if (!empty($_GET['redirect'])) {
        	$redirect = $_GET['redirect'];
        } else if (!empty($form['settings']['#redirect'])) {
        	$redirect = $form['settings']['#redirect'];
        } else if (!empty($form['#redirect'])) {
        	$redirect = $form['#redirect'];
        } else {
        	$redirect = url($_GET['q']);
        }
        
        header('Location: '. urldecode($redirect));
        exit();
      }
    }
  } else if (!$form['settings']['#noCache']) {
  	dd_set_form_cache($form);
  }
  
  if (empty($form['settings']['#prefix'])) {
    $form['settings']['#prefix'] = '<div class="' . $form_id . '">';
  }

  if (empty($form['settings']['#suffix'])) {
    $form['settings']['#suffix'] = '</div>';
  }

  $fields = dd_form_sort(dd_form_field($form, $form_values, $error));

  $form['#content'] = '';

  $tpl = DIDA_ROOT . '/' . $conf['themes'][$base_theme_name]['path'] . '/form/' . $form['#form_id'] . '.tpl.php';
  
  if (is_file($tpl)) {
    /**
     * 使用指定 tpl 输出表单 html
     * 在该 tpl 中，可自由布局，但必须保证 name 值不变
     * 注意：html 结构的更改，可能导致一些 js 功能失效，需要根据不同表单特别处理
     * 示例参考 themes/default/form/user_page_pass.tpl.php
     */
    ob_start();
    include $tpl;
    $form['#content'] .= ob_get_contents();
    ob_end_clean();
  } else {
    $form['#content'] .= dd_form_html($form, $fields);
  }

  $form['#content'] .= dd_form_hidden(array('#name' => 'form_id', '#value' => $form_id));
  $form['#content'] .= dd_form_hidden(array('#name' => 'form_token', '#value' => $form_token));

  return theme_get_element('form_get_html', $form);
}

// public form_get_html($form) {{{ 
/**
 * form_get_html
 * 输入表单 html
 * 
 * @param $array $form 
 * @access public
 * @return string
 */
function form_get_html($form) {
  $output = '';
  //ajax验证和提交将跳过系统默认验证，应在ajax页面调用dd_ajax_validate()来验证
  if (!empty($form['settings']['#ajax_submit']) && !empty($form['settings']['#ajax_submit']['des'])) {
    $output = '<div id="ajax_description" class="description"></div>';
  }
  $output .= '<form ' . dd_attributes($form['#args']) . '>';

  if (!empty($form['settings']['#title'])) {
    $output .= '<h3>'.$form['settings']['#title'].'</h3>';
  }

  if (!empty($form['settings']['#description'])) {
    $output .= '<div class="description form_description form_setting_description">' . $form['settings']['#description'] . '</div>';
  }

  $output .= $form['#content'];
  $output .= '</form>';
  return $form['settings']['#prefix'] . $output . $form['settings']['#suffix'];
}
// }}}

/**
 * 获取表单字段类型，可使用 hook_form_type_info() 来扩展字段类型，示例：
 *  mymodule_form_type_info(&$types) {
 *      $types['mytype'] = array(
 *        'name' => '我的自定义类型',
 *        'callback' => 'mymodule_form_type_mytype',
 *      );
 *  }
 * @access public
 * @return void
 */
function form_get_type_info() {
  static $info;

  if (!empty($info)) return $info;

  $types = array(
    'textfield' => array(
      'name' => t('system', '单行文本'), // 类型名称
      'callback' => 'dd_form_textfield', // 回调函数，该类型将用此函数处理，必需
      'description' => '单行文本字段，最长 255 个字符', // 类型描述
    ),
    'password' => array(
      'name' => t('system', '密码'),
      'callback' => 'dd_form_password'
    ),
    'captcha' => array(
      'name' => t('system', '验证码'),
      'callback' => 'dd_form_captcha'
    ),
    'textarea' => array(
      'name' => t('system', '多行文本'),
      'callback' => 'dd_form_textarea',
    ),
    'radio' => array(
      'name' => t('system', '单选按钮'),
      'callback' => 'dd_form_radio'
    ),
    'checkbox' => array(
      'name' => t('system', '复选框'),
      'callback' => 'dd_form_checkbox',
    ),
    'select' => array(
      'name' => t('system', '下拉单选框'),
      'callback' => 'dd_form_select'
    ),
    'selects' => array(
      'name' => t('system', '下拉复选框'),
      'callback' => 'dd_form_selects'
    ),
    'weight' => array(
      'name' => t('system', '权重'),
      'callback' => 'dd_form_weight'
    ),
    'ahah' => array(
      'name' => t('system', 'AJAX'),
      'callback' => 'dd_form_ahah'
    ),
    'submit' => array(
      'name' => t('system', '提交按钮'),
      'callback' => 'dd_form_submit',
    ),
    'button' => array(
      'name' => t('system', '按钮'),
      'callback' => 'dd_form_button'
    ),
    'hidden' => array(
      'name' => t('system', '隐藏字段'),
      'callback' => 'dd_form_hidden',
    ),
    'image' => array(
      'name' => t('system', '图像按钮'),
      'callback' => 'dd_form_image'
    ),
    'file' => array(
      'name' => t('system', '文件上传'),
      'callback' => 'dd_form_file',
    ),
    'value' => array(
      'name' => t('system', '内容显示'),
      'callback' => 'dd_form_value',
    ),
  );

  module_alter_all('form_type_info', $types);

  $info = $types;

  return $info;
}

/**
 * 提交按钮格式：
 * $form['submit'] = array(
 *  '#type' => 'submit',
 * 	'#submit' => 'user_register_submit',
 * 	'#value' => '确认注册'
 * );
 * 表单内允许多个提交按钮，但仅会处理当前被提交项。
 * 不论提交函数中的处理结果如何，均视为成功，并重定向。
 * 表单重定向顺序：
 * 	$_GET['redirect'] > $form['#redirect'] > $form['#action'] > $_GET['q']。
 * 基于此，数据合法性等应在表单验证环节处理，提交环节只负责数据写入或删除。
 * 可通过如下方式在提交函数中处理异常：
 * 	发现异常后重定向到某个页面，脱离默认表单处理流程。
*/
function form_save_submit(&$form, &$value) {
  foreach ($form['fields'] as $key => $field) {
    if (strpos($key, '#') === false) {
      if (($field['#type'] == 'submit' || $field['#type'] == 'image') && $value[$key]) {
        if ($function = $field['#submit']) {
          return $function($form, $value);
        }
      } else if (is_array($field)) {
        _form_save_submit($field, $value[$key], $value);
      }
    }
  }
}

function _form_save_submit($fields, $subvalue, &$value) {
  foreach ($fields as $key => $field) {
    if (strpos($key, '#') === false) {
      if (($field['#type'] == 'submit' || $field['#type'] == 'image') && $subvalue[$key]) {
        if ($function = $field['#submit']) {
          return $function($form, $value);
        }
      } else if (is_array($field)) {
        _form_save_submit($field, $subvalue[$key], $value);
      }
    }
  }
}

/**
 * ajax 验证
 * ___args settings 
 */
function dd_ajax_validate($uid, $form_id, $form_token, $clear = 1) {
	if ($cache = dd_get_form_cache($uid, $form_id, $form_token)) {
	  $form_values = $_POST;
	  
	  $_POST = NULL;
	  
	  $form = call_user_func_array($form_id, $cache['__args']);
	  $form['__args'] = $cache['__args'];
		module_alter_all('form_alter', $form_id, $form_token, $form);
		
		$form['#form_id'] = $form_id;
    $form['#form_token'] = $form_token;
    
    $form_values['_dida_values'] = (object) $form_values;
    
    $_POST = &$form_values;
    
		if ($error = dd_form_required($form, $form_values)) {
			return $error;
		} else if ($cache['settings'] && is_array($cache['settings']['#validate'])) {
      foreach ($cache['settings']['#validate'] as $function) {
        if ($error = $function($form, $form_values)) {
          return $error;
        }
      }
		}
    
    if ($clear) dd_del_form_cache($uid, $form_id, $form_token);
	}
}

/**
 * 获取表单缓存
 */
function dd_get_form_cache($uid, $form_id, $form_token) {
  $cid = $uid.$form_id.$form_token;
  
	static $cache;
	if (!isset($cache[$cid])) {
		$cache[$cid] = $_SESSION['form_token'][$cid];
	}
	return $cache[$cid];
}

/**
 * 生成表单缓存
 */
function dd_set_form_cache($form) {
  $data = array(
  	'form_token' => $form['#form_token'],
  	'settings' => $form['settings'],
  	'form_id' => $form['#form_id'],
  	'__args' => $form['__args'],
    '__timestamp' => $_SERVER['REQUEST_TIME']
  );
  
  $_SESSION['form_token'][$GLOBALS['user']->uid.$form['#form_id'].$form['#form_token']] = $data;
}

/**
 * 删除表单缓存
 */
function dd_del_form_cache($uid, $form_id, $form_token) {
  unset($_SESSION['form_token'][$uid.$form_id.$form_token]);
}

/**
 * 默认验证，验证表单合法性、必填字段
 */
function dd_form_required($form, $value) {
	global $user;
	
	if (!$form['settings']['#noCache']) {
	  $cache = dd_get_form_cache($user->uid, $form['#form_id'], $_POST['form_token']); // 获取表单初始化缓存数据
    if (!$cache || ($_POST['form_token'] != $cache['form_token'])) {
    	dd_set_message(t('system', '表单被非法修改，请重新进入页面'), 'error');
    	$error[] = t('system', '表单被非法修改，请重新进入页面');
      return $error;
    }
	}
  
  foreach ($form['fields'] as $key => $field) {
    if ($field['#type'] || isset($field['#value'])) {
      $field['#name'] = array($key);
      if ($t = _dd_form_required($form, $field, $value)) {
        $error[$key]['#error'] = $t;
      }
    } else if (is_array($field)) {
      $sub_field = array();
      foreach ($field as $child_key => $child) {
        if (strpos($child_key, '#') === false) {
          $sub_field[$child_key] = $child;
          $sub_field[$child_key]['#name'] = array($key);
        }
      }
      if ($t = _dd_form_required_subfield($form, $sub_field, $value)) {
        $error[$key] = $t;
      }
    }
  }
  return $error;
}

function _dd_form_required_subfield($form, $field, $value) {
  static $error;
  foreach ($field as $sub_key => $sub_field) {
    $sub_field['#name'] = array_merge($sub_field['#name'], array($sub_key));
    if ($sub_field['#type'] || isset($sub_field['#value'])) {
      if ($t = _dd_form_required($form, $sub_field, $value)) {
        $error[$sub_key]['#error'] = $t;
      }
    } else if (is_array($sub_field)) {
      $child_field = array();
      foreach ($sub_field as $child_key => $child) {
        if (strpos($child_key, '#') === false) {
          $child_field[$child_key] = $child;
          $child_field[$child_key]['#name'] = $sub_field['#name'];
        }
      }
      _dd_form_required_subfield($form, $child_field, $value);
    }
  }
  return $error;
}

function _dd_form_required($form, $field, $value) {
  $val = dd_get_value($value, $field['#name']);
  $name = array_shift($field['#name']);
  if (count($field['#name'])) {
    $name .= '['. implode('][', $field['#name']) .']';
  }
  $field['#name'] = $name;
	
  if (($field['#type'] != 'file') && $field['#required'] && ($val == '')) {
    $error[] =  t('system', '%text 不能为空', array('%text' => $field['#title']));
  }
  if ($field['#options']) {
    foreach ($field['#options'] as $key => $k) {
      $options[] = $key;
    }
  }
  switch ($field['#type']) {
    case 'select': case 'radio':
      if ($val != '' && !dd_in_array($val, $options)) {
        $er = true;
      }
    break;
    case 'selects': case 'checkbox':
      if ($options && is_array($val)) {
        foreach ($val as $check) {
          if (!dd_in_array($check, $options)) {
            $er = true;
            break;
          }
        }
      } else if ($val) {
        if ($val != 1) {
          $er = true;
        }
      }
    break;
    case 'captcha':
      if ($val != $_SESSION['captcha'][$form['#form_token'].$form['#form_id'] . $field['#name']]) {
        $error[] = t('system', '验证码不正确');
      }
    break;
    case 'hidden':
      if ($field['#constant'] && $field['#default_value'] != $val) {
        $error[] = true;
        dd_set_message(t('system', $field['#name']. '%string 出现非法选择，请重新进入', array('%string' => $field['#name'])), 'error');
      }
  }
  
  if ($er) {
    $error[] = t('system', '%string 出现非法选项，请重新提交', array('%string' => $field['#title']));
  }
	
  return $error;
}

/**
 * 表单字段转换为 html
 */
function dd_form_html($form, $fields) {
  if (empty($form['settings']['#theme'])) {
    $_fields = array();
    foreach ($fields as $key => $field) {
      if (is_array($field) && !empty($field['#groups'])) {
        // 分组名称，附加前缀，避免和字段冲突
        $_key = '__dida_form_groups_' . $field['#groups'];
        if (empty($_fields[$_key])) {
          $_fields[$_key] = !empty($form['groups']) ? $form['groups'][$field['#groups']] : array();
        }
        unset($field['#groups']);
        $_fields[$_key][$key] = $field;
      } else {
        $_fields[$key] =$field;
      }
    }
    
    foreach ($_fields as $key => $field) {
      if (strpos($key, '#') === false) {
        if (!empty($field['#html'])) {
          $output .= empty($field['#theme']) ? $field['#html'] : call_user_func($field['#theme'], $field);
        } else if (is_array($field)) {
          $output .= _dd_form_fieldset($field, 'prefix');
          $output .= empty($field['#html']) ? dd_form_html($form, $field) : call_user_func($field['#theme'], $field);
          $output .= _dd_form_fieldset($field, 'suffix');
        }
      }
    }
  } else {
    $output = call_user_func($form['settings']['#theme'], $form, _dd_form_html($fields));
  }
  return $output;
}

function _dd_form_html($fields) {
  
  foreach ($fields as $key => $field) {
    if (strpos($key, '#') === false) {
      if (is_array($field)) {
        if ($field['#html']) {
          if (!$field['#theme']) {
            $result[$key] = $field;
          } else if (function_exists($field['#theme'])) {
            $result[$key] = call_user_func($field['#theme'], $field);
          }
        } else {
          $result[$key] = _dd_form_html($field);
        }
      }
    }
  }
  return $result;
}

/**
 * 字段排序
 * @param array $fields
 */
function dd_form_sort($fields) {
  foreach ($fields as $key => $child) {
    //if (isset($child['#children'])) {
    if (strpos($key, '#') === false && is_array($child)) {
      uasort($child, 'dd_form_cmp');
      //$child = array_reverse($child, true);
      $childs = dd_form_sort_child($child);
      $fields[$key] = $childs;
    }
  }
  uasort($fields, 'dd_form_cmp');
  //$fields = array_reverse($fields, true);
  return $fields;
}

/**
 * 子字段排序
 * @param array $fields
 */
function dd_form_sort_child($fields) {
  foreach ($fields as $key => $child) {
    if (strpos($key, '#') === false && is_array($child)) {
      uasort($child, "dd_form_cmp");
      //$child = array_reverse($child, true);
      $childs = dd_form_sort_child($child);
      $fields[$key] = $childs;
    }
  }
  return $fields;
}

/**
 * 字段按权重排序
 */
function dd_form_cmp($a, $b) {
  $a_weight = is_array($a) && isset($a['#weight']) ? $a['#weight'] : 0;
  $b_weight = is_array($b) && isset($b['#weight']) ? $b['#weight'] : 0;
  
  if ($a_weight == $b_weight) {
    return 0;
  }
  
  return ($a_weight < $b_weight) ? -1 : 1;
}

/**
 * 解析表单
 */
function dd_form_field(&$form, $value, $error = array()) {
  
  foreach ($form['fields'] as $key => $field) {
    if (!is_array($field)) continue;
    if ($field['#type'] || isset($field['#value'])) {
      $field['#name'] = array($key);
      $fields[$key] = _dd_form_element($form, $field, $value, $error);
    } else {
      $parents = $sub_field = array();
      foreach ($field as $child_key => $child) {
        if (strpos($child_key, '#') === false) {
          $sub_field[$child_key] = $child;
          $sub_field[$child_key]['#name'] = array($key);
        } else {
          $parents[$child_key] = $child;
        }
      }
      $fields[$key] = dd_form_subfield($form, $sub_field, $value, $error[$key]);
      if ($parents && $fields[$key]) {
        $parents['#children'] = count($sub_field);
        $fields[$key] += $parents;
      }
      
			$fields[$key]['#weight'] = _dd_form_weight($form['#form_id'], $field['#weight']);
    }
  }
  return $fields;
}

/**
 * 多级
 */
function dd_form_subfield(&$form, $field, $value, $error) {
  foreach ($field as $sub_key => $sub_field) {
    if (!is_array($sub_field)) continue;
    $sub_field['#name'] = array_merge($sub_field['#name'], array($sub_key));
    if ($sub_field['#type'] || isset($sub_field['#value'])) {
      $fields[$sub_key] = _dd_form_element($form, $sub_field, $value, $error);
    } else {
      $parents = $child_field = array();
      foreach ($sub_field as $child_key => $child) {
        if (strpos($child_key, '#') === false) {
          $child_field[$child_key] = $child;
          $child_field[$child_key]['#name'] = $sub_field['#name'];
        } else {
          $parents[$child_key] = $child;
        }
      }
      $fields[$sub_key] = dd_form_subfield($form, $child_field, $value, $error[$sub_key]);
      if ($parents && $fields[$sub_key]) {
        $parents['#children'] = count($child_field);
        $fields[$sub_key] += $parents;
      }
      $fields[$sub_key]['#weight'] = _dd_form_weight($form['#form_id'], $sub_field['#weight']);
    }
  }
  return $fields;
}

/**
 * 获取字段值
 */
function dd_get_value($value, $name) {
  $count = count($name);
  for ($i = 0; $i < $count; ++$i) {
    if (isset($value[$name[$i]])) {
      if (is_array($value[$name[$i]])) {
        if ($i != ($count -1)) {
          $value = $value[$name[$i]];
        } else {
          return $value[$name[$i]];
        }
      } else {
        return $value[$name[$i]];
      }
    }
  }
  return false;
}

// public dd_get_error($value, $name) {{{ 
/**
 * dd_get_error
 * 获取指定字段错误信息
 * 
 * @param array $value 
 *  表单验证未通过时返回的错误信息
 * @param string $name 
 *  需获取错误信息的字段名称
 * @access public
 * @return array|bool
 */
function dd_get_error($value, $name) {
  $count = count($name);
  for ($i = 0; $i < $count; ++$i) {
    if ($value[$name[$i]]) {
      if ($value[$name[$i]]['#error']) {
        return $value[$name[$i]]['#error'];
      } else if (is_array($value[$name[$i]])) {
        $value = $value[$name[$i]];
      }
    }
  }
  return false;
}
// }}}

function _dd_form_weight($form_id, $weight = 0) {
	static $i = array();
	if ($weight == 0) {
		$weight += $i[$form_id];
	}
	++$i[$form_id];
	return $weight;
}

function _dd_form_fieldset($field, $type) {
  if ($type == 'prefix') {
    if ($field['#fieldset_prefix']) {
      if (!$field['#fieldset_attributes'] || !$field['#fieldset_attributes']['class']) {
        $field['#fieldset_attributes']['class'] = 'collapsible';
      } else {
        $field['#fieldset_attributes']['class'] .= ' collapsible';
      }
      if ($field['#fieldset_prefix'] == 'asc') {
        $output .= '<fieldset '.dd_attributes($field['#fieldset_attributes']) .'><legend class="collapse-processed asc">';
    	} else {
        $field['#fieldset_attributes']['class'] .= ' fieldset-hide';
    		$output .= '<fieldset '.dd_attributes($field['#fieldset_attributes']) .'><legend class="collapse-processed desc">';
    	}
    	$output .= '<a href="#">'.$field['#fieldset_legend'].'</a></legend><div class="fieldset-wrapper">';
    }
    if (!$field['#type'] && $field['#description']) {
      $output .= '<div class="description form_description form_setting_description form_fieldset_description">'.$field['#description'].'</div>';
    }
  } else if ($field['#fieldset_suffix']) {
  	$output = '</div></fieldset>';
  }
  
  return $output;
}

// protected _dd_form_element(&$form, $field, $value, $error) {{{ 
/**
 * _dd_form_element
 * 解析一个表单字段，将字段定义参数转换为 html 
 * 
 * @param array $form 
 *  表单对象
 * @param array $field 
 *  一个表单字段
 * @param array $value 
 *  表单提交值 
 * @param array $error 
 *  表单验证错误值
 * @access protected
 * @return string
 */
function _dd_form_element(&$form, $field, $value, $error) {
  $types = form_get_type_info();

  $field['#weight'] = _dd_form_weight($form['#form_id'], $field['#weight']);
  $field['#parents'] = $field['#name'];
  $name = array_shift($field['#name']);

  if (count($field['#name'])) {
    $name .= '['. implode('][', $field['#name']) .']';
  }

  $field['#name'] = $name;
  
  if ($value['form_id'] == $form['#form_id']) {
    $val = dd_get_value($value, $field['#parents']);
    if ($val !== false && !dd_in_array($field['#type'], array('button', 'submit', 'image'))) {
      $field['#value'] = $val;
    }
    $field['#error'] = dd_get_error($error, $field['#parents']);
  } else if (!isset($field['#value'])) {
    $field['#value'] = isset($field['#default_value']) ? $field['#default_value'] : '';
  }
  
  if (empty($field['#attributes'])) $field['#attributes'] = array();

  $field['__name'] = str_replace(array('[', ']'), array('_'), $field['#name']);

  $class = array();

  if (!empty($field['#attributes']['class'])) {
    $class[] = $field['#attributes']['class'];
  }
  
  if (!empty($field['#required'])) $class[] = 'required';
  
  if (in_array($field['#type'], array('textfield', 'password', 'captcha'))) {
    $class[] = 'form_text';
  }
  
  if (!isset($field['#type'])) $field['#type'] = 'value';
  
  $class[] = $field['#type'];
  $class[] = ' form_' . $field['#type'] . '_name_' . $field['__name'];

  if ($class) $field['#attributes']['class'] = implode(' ', $class);

  $field['#form_id'] = $form['#form_id'];
  
  if (!empty($field['#disabled'])) $field['#attributes']['disabled'] = 'disabled';
  
  if (!empty($field['#fieldset_prefix'])) {
    $field['#fieldset_prefix_html'] = _dd_form_fieldset($field, 'prefix');
  }

  if (!empty($field['#fieldset_suffix'])) {
    $field['#fieldset_suffix_html'] = '</div></fieldset>';
  }

  if (!empty($field['#value']) && empty($field['#type'])) {
    $field['#type'] = 'value';
  }
  
  if (!empty($types[$field['#type']]) && function_exists($types[$field['#type']]['callback'])) {
    if (empty($field['#attributes'])) $field['#attributes'] = array();

    if ($field['#type'] == 'file' && empty($form['#args']['enctype'])) {
      $form['#args']['enctype'] = 'multipart/form-data';
    }
    
    if (empty($field['#attributes']['id'])) {
      $field['#attributes']['id'] = $field['#form_id'] . '_type_' . $field['__name'];
    }
    
    if (empty($field['#attributes']['class'])) {
      $field['#attributes']['class'] = 'form_value_' . $field['#type'];
    } else {
      $field['#attributes']['class'] .= ' form_value_' . $field['#type'];
    }
  }

  $field = form_field_element($field, $form);

  form_set_ahah($field);

  return $field;
}
// }}}

// public form_field_element($field, $form, $html) {{{ 
/**
 * form_field_element
 * 将表单字段数据分解为可供主题使用的元素
 * 
 * @param array $field 
 *  表单的一个字段
 * @param array $form
 *  表单
 * @param bool $html
 *  返回整个字段或只返回 html
 * @access public
 * @return array|string
 */
function form_field_element($field, &$form, $html = false) {
  $types = form_get_type_info();

  $keys = array(
    '#description', '#prefix', '#suffix', '#field_prefix', '#field_suffix', '#fieldset_suffix_html', '#fieldset_prefix_html'
  );

  $field['#element'] = array(
    '#label' => theme_get_element('dd_form_label', $field),
    '#error' =>  theme_get_element('dd_form_error', $field),
  );

  foreach ($keys as $key) {
    $field['#element'][$key] = !empty($field[$key]) ? $field[$key] : '';
  }

  if (!empty($field['#type']) && !empty($types[$field['#type']]['callback'])) {
    $field['#element']['#content'] = theme_get_element($types[$field['#type']]['callback'], $field, $form);
  }

  $field = theme_get_element('form_wrapper_element', $field, $form);
  return !$html ? $field : $field['#html'];
}
// }}}

// public form_wrapper_element($field, $form) {{{ 
/**
 * form_wrapper_element
 * 将表单字段元素组合为 html
 * 
 * @param array $field 
 * @param array $form 
 * @access public
 * @return array
 */
function form_wrapper_element($field, $form = array()) {
  $element = $field['#element']['#label'] . $field['#element']['#field_prefix'];
  $element .= $field['#element']['#content'];
  $element .= $field['#element']['#field_suffix'];
  $element .= $field['#element']['#error'];
 
  if (!empty($field['#element']['#description'])) {
    $element .= '<span class="description form_description">' . $field['#element']['#description'] . '</span>';
  }

  if (isset($field['#prefix']) || isset($field['#suffix'])) {
    $element = $field['#element']['#prefix'] . $element . $field['#element']['#suffix'];
  } else if (empty($form['settings']['#theme']) && $field['#type'] != 'hidden') {
    $element = '<div id="' . $field['#form_id'] . '_' . $field['__name'] . '" class="form_item form_item_' . $field['#type'] . '">' . $element . '</div>';
  }

  $field['#html'] = $field['#element']['#fieldset_prefix_html'] . $element . $field['#element']['#fieldset_suffix_html'];

  return $field;
}
// }}}

// public dd_form_error($field) {{{ 
/**
 * dd_form_error
 * 字段错误信息输出
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_error($field) {
  if (is_array($field['#error']) && !empty($field['#error'][0])) {
    $output = '<span class="form_error">';
    foreach ($field['#error'] as $error) {
      $output .= '<span>' . $error . '</span>';
    }
    $output .= '</span>';
    return $output;
  }
}
// }}}

// public dd_form_value($field) {{{ 
/**
 * dd_form_value
 * 内容显示字段
 * 
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_value($field) {
  return $field['#value'];
}
// }}}

// public dd_form_textfield($field) {{{ 
/**
 * dd_form_textfield
 * 单行文本字段
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_textfield($field) {
  if (!empty($field['#autocomplete'])) form_set_auto($field);
  
  $field['#attributes']['name'] = $field['#name'];
  $field['#attributes']['value'] = check_plain($field['#value']);
  $field['#attributes']['type'] = 'text';

  return '<input' . dd_attributes($field['#attributes']) . '/>';
}
// }}}

// public dd_form_hidden($field) {{{ 
/**
 * dd_form_hidden
 * 隐藏字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_hidden($field) {
  return '<input type="hidden" name="' . $field['#name'] . '" value="' . $field['#value'] . '"' . dd_attributes($field['#attributes']) . '/>';
}
// }}}

// public dd_form_weight($field) {{{ 
/**
 * dd_form_weight
 * 权重字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_weight($field) {

  if (empty($field['#options'])) {
    $start = !empty($field['#start']) && is_numeric($field['#start']) ? $field['#start'] : 0;
    $end = !empty($field['#end']) && is_numeric($field['#end']) ? $field['#end'] : 20;
    $options = array();

    foreach (range($start, $end) as $value) {
      $options[$value] = $value;
    }
    
    $field['#options'] = $options;
  }
  
  if (empty($field['#value']))	$field['#value'] = 0;
   
  return dd_form_select($field);
}
// }}}

// public dd_form_password($field) {{{ 
/**
 * dd_form_password
 * 密码字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_password($field) {
  $output = '<input type="password" name="' . $field['#name'] . '" value="' . $field['#value'] . '"';
  $output .= dd_attributes($field['#attributes']) . '/>';
  return $output;
}
// }}}

// public dd_form_textarea($field) {{{ 
/**
 * dd_form_textarea
 * 多行文本字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_textarea($field) {
  if (empty($field['#attributes']['rows'])) $field['#attributes']['rows'] = 10;
  if (empty($field['#attributes']['cols'])) $field['#attributes']['cols'] = 60;
  $output = '<div class="form_textarea"><textarea name="' . $field['#name'] . '"';
  $output .= dd_attributes($field['#attributes']).'>' . $field['#value'] . '</textarea></div>';
  return $output;
}
// }}}

// public dd_form_captcha($field, $form = NULL) {{{ 
/**
 * dd_form_captcha
 * 验证码字段
 * 
 * @param array $field 
 *  一个表单字段
 * @param array $form 
 *  表单对象
 * @access public
 * @return string
 */
function dd_form_captcha($field, $form = NULL) {
  
  // 检查是否有字体文件
  if (!system_is_captcha_ttf()) unset($field['#captcha']['image']);

  $key = $form['#form_token'] . $field['#name'] . $form['#form_id'];

  $values = array(
    'name' => $field['#name'],
    'type' => !empty($field['#captcha']['type']) ? $field['#captcha']['type'] : 0,
    'form_id' => $form['#form_id'],
    'tag' => $form['#form_token'],
    'mosaic' => $field['#captcha']['mosaic'],
    'image' => !empty($field['#captcha']['image']) ? $field['#captcha']['image'] : 0,
  );

  dd_add_js(array('captcha' => array($key => array(
    'data' => dd_encrypt(serialize($values)),
    'image' => $values['image'],
  ))), 'setting');

  if (strpos($field['#attributes']['class'], 'form_field_input_captcha') === false) {
    $field['#attributes']['class'] .= ' form_field_input_captcha';
  }

  $html = '<input type="text" name="' . $field['#name'] . '" value="';
  $html .= $field['#value'] . '"' . dd_attributes($field['#attributes']).'/>';
  $html .= '<span class="form_captcha" alt="' . $key . '" title="' . t('system', '点击换一个验证码') . '">';

  if (!empty($field['#captcha']['show'])) {
    // 默认显示验证码
    require_once DIDA_ROOT . '/modules/system/system.captcha.inc';
    $html .= captcha_get($values); 
  } else {
    // 需点击显示
    $html .= t('system', '获取验证码');
  }

  $html .= '</span>';

  return $html;
}
// }}}

// public dd_form_checkbox($field) {{{ 
/**
 * dd_form_checkbox
 * 复选框字段
 * 
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_checkbox($field) {
  $output = '';
  
  if (!empty($field['#options'])) {
    $id = $field['#attributes']['id'];
    $field['#attributes']['class'] .= ' '.$id;

    foreach ($field['#options'] as $key => $data) {
      $output .= '<span class="form_checkbox_option" alt="'.$id.'"><input';

      if (is_array($field['#value']) && dd_in_array($key, $field['#value'])) {
        $output .= ' checked="checked"';
      }

      $field['#attributes']['id'] = $id .'_'. $key;
      
      $att = $field['#attributes'];

      if (is_array($data)) {
        foreach ($data as $_key => $value) {
          if ($_key == 'data' || $_key == 'name') {
            $text = $value;
          } else {
            $att[$_key] = $value;
          }
        }
      } else {
        $text = $data;
      }

      $output .= ' value="' . $key . '" name="' . $field['#name'] . '[' . $key . ']" type="checkbox"';

      $output .= dd_attributes($att) . '/><span class="option_label">' . $text . '</span></span>';
    }
  } else {
    $output .= '<input';
    if ($field['#value']) $output .= ' checked="checked"';
    $output .= ' value="1" name="' . $field['#name'] . '" type="checkbox"' . dd_attributes($field['#attributes']) . '/>';
  }

  return $output;
}
// }}}

// public dd_form_select($field) {{{ 
/**
 * dd_form_select
 * 下拉框字段
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_select($field) {
  $output = '<select name="' . $field['#name'] . '"' . dd_attributes($field['#attributes']) . '>';

  foreach ($field['#options'] as $key => $data) {
    $output .= '<option';

    if (isset($field['#value']) && (string) $field['#value'] == (string) $key) $output .= ' selected="selected"';

    if (is_array($data)) {
      $text = !empty($data['name']) ? $data['name'] : $data['data'];
      unset($data['name'], $data['data']);
      if (!empty($data)) $output .= dd_attributes($data);
    } else {
      $text = $data;
    }
   
    $output .= ' value="' . $key . '">' . $text . '</option>';
  }

  $output .= '</select>';

  return $output;
}
// }}}

// public dd_form_selects($field) {{{ 
/**
 * dd_form_selects
 * 下拉复选框字段
 * 
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_selects($field) {
  $output = '<select name="'.$field['#name'].'[]"' . dd_attributes($field['#attributes']) . ' multiple="multiple">';
  
  foreach ($field['#options'] as $key => $data) {
    $output .= '<option';

    if ($field['#value'] && dd_in_array($key, $field['#value'])) {
      $output .= ' selected="selected"';
    }
 
    if (is_array($data)) {
      $text = !empty($data['name']) ? $data['name'] : $data['data'];
      unset($data['name'], $data['data']);
      if (!empty($data)) $output .= dd_attributes($data);
    } else {
      $text = $data;
    }
   
    $output .= ' value="' . $key . '">' . $text . '</option>';   
  }

  $output .= '</select>';

  return $output;
}
// }}}

/**
 */
// public dd_form_radio($field) {{{ 
/**
 * dd_form_radio
 * 
 * 单选框字段
 * @param array $field
 * @access public
 * @return string
 */
function dd_form_radio($field) {
  $output = '';

  if (is_array($field['#options'])) {
    $id = $field['#attributes']['id'];

    if (!empty($field['#attributes']['class'])) {
      $field['#attributes']['class'] .= ' ' . $id;
    } else {
      $field['#attributes']['class'] = $id;
    }

    foreach ($field['#options'] as $key => $data) {
      $field['#attributes']['id'] = $id . '_' . $key;
      $att = $field['#attributes'];

      if (is_array($data)) {
        foreach ($data as $_key => $value) {
          if ($_key == 'data' || $_key == 'name') {
            $text = $value;
          } else {
            $att[$_key] = $value;
          }
        }
      } else {
        $text = $data;
      }

      $output .= '<span class="form_radio_field"><input';
      
      if ((string) $field['#value'] == (string) $key) $output .= ' checked="checked"';

      $output .= ' value="' . $key . '" name="' . $field['#name'] . '" type="radio"';
      $output .= dd_attributes($att) . '/>';
      $output .= '<span class="form_radio_text">' . $text . '</span></span>';
    }
  } else {
    $output .= '<span class="form_radio_field"><input';
    if ($field['#value']) $output .= ' checked="checked"';
    $output .= ' value="1" name="'.$field['#name'] . '" type="radio"' . dd_attributes($field['#attributes']);
    $output .= '/></span>';
  }

  return $output;
}
// }}}

// public dd_form_file($field) {{{ 
/**
 * dd_form_file
 * 上传字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_file($field) {
  return file_get_form($field);
}
// }}}

/**
 * @return string
 */
// public dd_form_button($field) {{{ 
/**
 * dd_form_button
 * 
 * 按钮字段
 * @param array $field
 * @access public
 * @return string
 */
function dd_form_button($field) {
  return '<input type="button" name="' . $field['#name'] . '" value="' . ($field['#value'] ? $field['#value'] : 'Button').'"' . dd_attributes($field['#attributes']) . '/>';
}
// }}}

// public dd_form_submit($field) {{{ 
/**
 * dd_form_submit
 * 提交按钮字段
 * 
 * @param array $field 
 *  一个表单字段
 * @access public
 * @return string
 */
function dd_form_submit($field) {
  return '<input type="submit" name="' . $field['#name'] . '" value="' . ($field['#value'] ? $field['#value'] : t('system', '确认提交')) . '"' . dd_attributes($field['#attributes']) . '/>';
}
// }}}

// public dd_form_image($field) {{{ 
/**
 * dd_form_image
 * 图片按钮字段
 * 
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_image($field) {
  return '<input type="image" name="' . $field['#name'] . '" src="' . $field['#src'] . '"' . dd_attributes($field['#attributes']) . '/>';
}
// }}}

// public dd_form_label($field) {{{ 
/**
 * dd_form_label
 * 表单字段 label
 * 
 * @param array $field 
 * @access public
 * @return string
 */
function dd_form_label($field) {
  $output = '';
  if (!empty($field['#title'])) {
    $output = '<label class="dd_label dd_label_' . $field['#type'] . '" for="' . $field['#name'] . '">';
    if (!empty($field['#title'])) $output .= $field['#title'] . '：';
    if (!empty($field['#required'])) $output .= '<span class="form_required" title="' . t('system', '此项不能为空') . '">*</span>';
    $output .= '</label>';
  }
  return $output;
}
// }}}

function form_set_auto($field = NULL) {
  if ($field['#autocomplete']) {
    dd_jqui();
    if (empty($field['#autocomplete']['dom']) && !empty($field['#attributes']['id'])) {
      $field['#autocomplete']['dom'] = '#' . $field['#attributes']['id'];
    }
    foreach ($field['#autocomplete'] as $key => $value) {
      if ($value) {
        $auto_val[$key] = $value;
      }
    }
    dd_add_js(array('auto' => array($auto_val)), 'setting');
  }
}

function form_set_ahah($field = NULL) {
  if (isset($field['#ahah']['url']) && !isset($field['#ahah']['event'])) {
    switch ($field['#type']) {
      case 'textfield':
        $field['#ahah']['event'] = 'keyup';
      break;
      case 'select': case 'selects': case 'radio': case 'checkbox':
        $field['#ahah']['event'] = 'change';
      break;
      case 'submit': case 'image': case 'button':
        $field['#ahah']['event'] = 'click';
    }
  }
  if (isset($field['#attributes']['id']) && isset($field['#ahah']['url']) && isset($field['#ahah']['event'])) {
    dd_add_js('misc/ahah.js');
    dd_add_js('misc/jquery.form.js');
    /*
      event => 触发事件
      url => 请求url
      type => 表单提交类型
      beforeSubmit => 提交前回调函数
      success => 返回值处理函数
      dataType => 返回值类型
      clearForm => 表单提交成功后是否清除表单
      target => 页面更新的元素
    */
    foreach ($field['#ahah'] as $key => $value) {
      if ($value) {
        $ahah_val[$key] = $value;
      }
    }
    $ahah_val['id'] = $field['#attributes']['id'];
    dd_add_js(array('ahah' => array($field['#attributes']['id'] => $ahah_val)), 'setting');
  }
}

/**
 * 验证邮箱
 */
function form_vali_mail($mail) {
  if (!preg_match('/([a-z0-9_-])+@([a-z0-9_-])+.[a-z].*/i', $mail)) {
    return t('system', '邮箱格式不正确');
  }
}

/**
 * 验证手机号码
 */
function form_vali_mobile($mobile) {
	if (strlen($mobile) != 11) {
		$error = t('system', '手机号码有误');
	} else if (preg_match('/[^0-9]/', $mobile)) {
		$error = t('system', '手机号码只能是数字');
	}
  return $error;
}

function dd_form_inline($form) {
  return '<div class="dd_form_inline">' . $form . '</div>';
}

function dd_label($v, $class = NULL) {
  return '<span class="dd_label ' . $class . '">' . $v . '</span>';
}

/**
 * meta 表单字段，使用方法：
 * $form['fields']['meta'] = dd_get_form_meta('asc', $value, array('title' => '页面标题');
 * 
 * @param (string) $sort
 *  字段组默认展开或闭合
 * @param (array) $value
 *  字段默认值
 * @param (array) $title
 *  字段标题，如：array('title' => '页面标题')
 * @return (array) 表单字段
 * 
 */
function dd_get_form_meta($sort = 'desc', $value = array(), $title = array()) {
  $form = array(
    '#fieldset_prefix' => $sort,
    '#fieldset_legend' => !empty($title['fieldset']) ? $title['fieldset'] : t('system', 'meta 参数'),
    '#fieldset_suffix' => 1,
    '#fieldset_attributes' => array('class' => 'dd_get_form_meta_fields'),
    '#fieldset' => true,
  );
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => !empty($title['title']) ? $title['title'] : 'title',
    '#attributes' => array('size' => 50),
    '#default_value' => $value['title'],
  );
  
  $form['keywords'] = array(
    '#type' => 'textfield',
    '#title' => !empty($title['keywords']) ? $title['keywords'] : 'keywords',
    '#attributes' => array('size' => 50),
    '#default_value' => $value['keywords'],
  );
  
  $form['robots'] = array(
    '#type' => 'textfield',
    '#title' => !empty($title['robots']) ? $title['robots'] : 'robots',
    '#attributes' => array('size' => 50),
    '#default_value' => $value['robots'],
    '#description' => t('system', 'all|none|index|noindex|follow|nofollow')
  );
  
  $form['author'] = array(
    '#type' => 'textfield',
    '#title' => !empty($title['author']) ? $title['author'] : 'author',
    '#attributes' => array('size' => 50),
    '#default_value' => $value['author'],
  );
  
  $form['generator'] = array(
    '#type' => 'textfield',
    '#title' => !empty($title['generator']) ? $title['generator'] : 'generator',
    '#attributes' => array('size' => 50),
    '#default_value' => $value['generator'],
  );
  
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => !empty($title['description']) ? $title['description'] : 'description',
    '#default_value' => $value['description'],
    '#attributes' => array('rows' => 5),
  );
  
  return $form;
}

/**
 * 便捷搜索表单
 * @param string $name
 *  关键词字段名称
 * @param array $form
 *  可传入自定义字段
 * @param string $title
 *  输入字段显示名称
 */
function form_admin_search($name = 'keyword', $form = array(), $title = ''){
  $form['settings']['#validate'][] = 'form_admin_search_validate';
  $form['settings']['#attributes']['class'] = 'form-inline';

  if (empty($form['fields'][$name])) {
    $form['fields'][$name] = array(
      '#default_value' => !empty($_GET[$name]) ? urldecode($_GET[$name]) : '',
      '#type' => 'textfield',
      '#title' => ($title ? $title : t('system', '搜索')),
      '#required' => true,
      '#prefix' => '<div class="form_admin_search">'
    );
  }
  
  $form['fields'][$name]['#prefix'] = '<div class="form_admin_search">';

  $form['fields']['__field'] = array(
    '#type' => 'hidden',
    '#default_value' => $name,
    '#constant' => 1
  );
  
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#suffix' => '</div>'
  );
  
  return $form;
}

function form_admin_search_validate(&$form, &$value){
  dd_goto($_GET['q'], dd_query_string_encode($_REQUEST, array_merge(array('q', 'page', 'submit', 'form_id', 'form_token'), array_keys($_COOKIE))));
}

/**
 * 获取日期开始和结束字段
 * @param string $title 
 *  字段名称
 * @param string $type 
 *  表单 method，用于获取默认值
 * @param string $start
 *  默认开始日期，如：2010-12-24
 * @param string $end
 *  默认结束日期，如：2010-12-27
 * @param string $return
 *  需要返回的格式 array|string，前者可用于表单 $form，后者可直接输出到页面
 * @access public
 * @return array|string
 */
function form_date_start_or_end($title, $type = 'get', $return = 'string', $start = NULL, $end = NULL) {

  $default = $type == 'get' ? $_GET : $_POST;
  $start = empty($default['date_start']) ? $start : $default['date_start'];
  $end = empty($default['date_end']) ? $end : $default['date_end'];

  $fields = array(
    'date_start' => array(
      '#title' => $title,
      '#type' => 'textfield',
      '#default_value' => $start,
      '#value' => $start,
      '#attributes' => array('class' => 'form_text ui_date_click {dateISO: true}')
     ),
     'date_end' => array(
      '#type' => 'textfield',
      '#default_value' => $end,
      '#value' => $end,
      '#attributes' => array('class' => 'form_text ui_date_click {dateISO: true}')
    ),
  );

  dd_add_js('misc/form.js');
  
  dd_add_js(array('uidate' => array(
    array('dom' => 'input[name="date_start"]', 'start' => 'ok', 'end' => 'input[name="date_end"]'),
    array('dom' => 'input[name="date_end"]', 'end' => 'ok', 'start' => 'input[name="date_start"]')
  )), 'setting');

  dd_jqui();

  if ($return == 'string') {
    $fields['date_start']['#name'] = 'date_start';
    $fields['date_end']['#name'] = 'date_end';
    return '<div class="dd_form_date_start_or_end_wrapper">' . form_field_element($fields['date_start']) .' -- '. form_field_element($fields['date_end']) . '</div>';
  } else {
    $fields['date_start']['#prefix'] = '<div class="form_inline">';
    $fields['date_end']['#suffix'] = '</div>';
    return $fields;
  }
}

// public dd_form_ajax_form($url, $title = '', $att = NULL) {{{ 
/**
 * dd_form_ajax_form
 * 获得一个 ajax 提交按钮
 * 
 * @param string $url 
 *  表单提交处理页面
 * @param string $title
 *  按钮名称
 * @param array $att 
 *  按钮属性
 * @access public
 * @return string
 */
function dd_form_ajax_form($url, $title = '', $att = NULL) {
 if (empty($title)) $title = t('system', '确认提交');

 $att['alt'] = $url;
 $att['class'] = 'dd_form_ajax_form_button';

 dd_add_js('misc/jquery.form.js');
 
 return dd_form_button(array(
		'#name' => 'dd_form_ajax_form_button[]',
		'#value' => $title,
		'#attributes' => $att
	));
}
// }}}

/**
 * 全选
 * @param array $att
 * @param string $value
 */
function form_util_all_check($att = NULL, $value = NULL) {
  $output = '<input type="checkbox" name="form_field_check_all[]" ';
  
  if (!isset($att['class'])) {
    $att['class'] = 'form_field_all_check';
  }
  if (!isset($att['alt'])) {
    $att['alt'] = 'form_field_check_dom';
  }
  
  $output .= dd_attributes($att);
  $output .= ' />';
  
  $output .= !isset($value) ? t('system', '全选') : $value;
  return $output;
}

/**
 * 配合全选按钮，class 对应全选按钮 alt 属性
 * @param array $att
 */
function form_util_all_check_dom($att = NULL) {
  $output = '<input type="checkbox" ';
  
  if (empty($att['name'])) {
    $output .= 'name="form_field_all_list_check[]" ';
  }

  if (!isset($att['class'])) {
    $att['class'] = 'form_field_check_dom';
  }
  
  $output .= dd_attributes($att);
  $output .= ' />';
  
  return $output;
}

/**
 * 配合全选，通过 ajax 执行批量操作，rel 对应被选中对象的 class 属性
 * @param array $att
 */
function form_util_all_check_button($att = NULL) {
  $output = '<input name="form_field_all_button[]" type="button" ';
  if (!isset($att['value'])) {
    $att['value'] = t('system', '确认删除');
  }
  
  if (!isset($att['rel'])) {
    $att['rel'] = 'form_field_check_dom';
  }
  
  $att['class'] = 'form_field_all_button';
  
  $output .= dd_attributes($att);
  $output .= ' />';
  return $output;
}
