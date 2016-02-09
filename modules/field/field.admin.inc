<?php
// $Id$

function _field_lists($module = NULL, $type = NULL, $filter_header = array()) {
  if (!empty($_POST['field_field_id'])) {
    if (isset($_POST['field_change_weight']) && is_numeric($_POST['field_change_weight'])) {
      if (db_exec('UPDATE {fields} SET weight = ? WHERE field_id = ?', array($_POST['field_change_weight'], $_POST['field_field_id']))) {
        field_get_cache_field(NULL, 1);
        field_set_cache_term();
        echo 'ok';
      }
      exit;
    }
    if (isset($_POST['field_change_required']) && is_numeric($_POST['field_change_required'])) {
      if (db_exec('UPDATE {fields} SET required = ? WHERE field_id = ?', array($_POST['field_change_required'], $_POST['field_field_id']))) {
        field_get_cache_field(NULL, 1);
        field_set_cache_term();
        echo 'ok';
      }
      exit;
    }
  }
  
  if (!empty($_GET['field_field_id'])) {
    if ($field = field_load($_GET['field_field_id'])) {
      switch ($_GET['field_op']) {
        case 'custom':
          /**
           * 预留路径，由定义字段类型的模块操作
           * 比如，多级分类可在此提供下级分类管理界面
           * 注意：仅回调定义当前字段类型的模块的 hook_field_call_custom()
           */
          $types = field_get_call_lists();
          $function = $types[$field->field_type]['module']. '_field_call_custom';
          if (function_exists($function)) {
            return $function($field);
          } else {
            dd_set_message(t('field', '错误的请求'));
            dd_goto(dd_get_history());
          }
        break;
        case 'update':
          if (!$field->locked) {
            return dd_get_form('field_form', $field);
          } else {
            dd_set_message(t('field', '这个字段不允许编辑'), 'error');
          }
        break;
        case 'export': // 导出
          return field_export($field->field_id);
        break;
        case 'delete':
          /**
           * 删除字段
           */
          field_delete($field);
          dd_goto(dd_get_history());
        exit;
      }
    } else {
      dd_set_message(t('field', '错误的访问'), 'error');
      return;
    }
  }
   
  $types = field_get_call_lists();
  
  $header = array(
    'name' => array('data' => t('field', '名称'), 'field' => 'name'),
    'type' => array('data' => t('field', '类型'), 'field' => 'field_type'),
    'index' => array('data' => t('field', '索引'), 'field' => 'field_key'),
    'groups' => array('data' => t('field', '分组'), 'field' => 'groups'),
    'required' => array('data' => t('field', '必需'), 'field' => 'required'),
    'weight' => array('data' => t('field', '权重'), 'field' => 'weight'),
    'links' => t('field', '操作')
  );

  $args = array(); 

  $query = 'SELECT * FROM {fields}';

  if ($module && $type) {
    $query .= ' WHERE module = ? AND type = ?';
    $args = array($module, $type);
  } else if ($module) {
    $args = array($module);
    $query = ' WHERE module = ?';
  }

  $query .= table_sql($header);
  
  if (!$fetch = pager_query($query, 20, 0, NULL, $args)) {
    return system_no_content();
  }
 
  if (!empty($filter_header)) $header = array_intersect_key($header, array_flip($filter_header));

  if ($get = dd_query_string_encode($_GET, array_merge(array('q', 'field_field_id'), array_keys($_COOKIE)))) {
    $get .= '&';
  }
  
  foreach ($fetch as $field) {
    $links = array();
    if (!$field->locked) {
      $links['locked'] = l(t('field', '编辑'), $_GET['q'], array('query' => $get.'field_op=update&field_field_id='.$field->field_id.'&redirect='.rawurlencode($_SERVER['REQUEST_URI'])));
    }

    if ($field->deleted) {
      $links['deleted'] = l(t('field', '删除'), $_GET['q'], array('query' => $get.'field_op=delete&field_field_id='.$field->field_id, 'attributes' => array('class' => 'confirm')));
    }

    $links['export'] = l(t('field', '导出'), $_GET['q'], array('query' => $get.'field_op=export&field_field_id='.$field->field_id));
    module_alter_all('field_call_links', $field, $links, $get);

    $rows = array();

    if (!empty($header['name'])) {
      $rows[] = $field->name;
    }
    
    if (!empty($header['type'])) {
      $rows[] = $types[$field->field_type]['title'];
    }
    
    if (!empty($header['type'])) {
      $rows[] = $field->field_key;
    }
     
    if (!empty($header['groups'])) {
      $rows[] = $field->groups ? $field->groups : t('field', '默认');
    }
    
    if (!empty($header['required'])) {
      $rows[] = dd_form_select(
        array(
          '#name' => 'field_list_required',
          '#value' => $field->required,
          '#attributes' => array('alt' => $field->field_id),
          '#options' => array(t('field', '否'), t('field', '是'))
        )
      );
    }
    
    if (!empty($header['weight'])) {
      $rows[] = dd_form_weight(
        array(
          '#name' => 'field_list_weight',
          '#value' => $field->weight,
          '#attributes' => array('alt' => $field->field_id),
          '#start' => -20,
          '#end' => 20,
        )
      );
    }

    if (!empty($header['links'])) {
      $rows[] = dd_imp($links);
    }

    $table[] = $rows;
  }
  
  $output = theme('table', $header, $table, array('id' => 'field_admin_list'));
  $output .= pager_view();
  return $output;
}

function _field_form($field) {
  if (!$field->type || !$field->module) {
    dd_set_message(t('field', '参数不完整，无法调用表单'), 'error');
    return;
  }
  
  if (empty($field->field_id) && !empty($_GET['field_field_import']) && !empty($_SESSION['field_import'])) {
    $field = (object)array_merge((array)$_SESSION['field_import'], (array)$field);
  }
  
  if (!$group = module_invoke($field->module, 'field_call_group', $field->type)) {
    dd_set_message(t('field', '字段组未定义，无法调用表单'), 'error');
    return;
  }

  // 允许的字段类型
  $options = field_get_type_options($field->type, $group);

  /**
   * 添加时，必须先选择字段类型，以便于回调自定义项目
   */
  if (!$field->field_type && !empty($_GET['field_field_type'])) {
    $field->field_type = $_GET['field_field_type'];
  }
  
  if (!$field->field_type || !$field_type = field_get_call_lists($field->field_type)) {
    $form['fields']['field_type'] = array(
      '#type' => 'select',
      '#title' => t('field', '请选择字段类型'),
      '#required' => 1,
      '#default_value' => $field->field_type,
      '#options' => $options 
    );

    if (empty($group['disable_import'])) {
      $form['fields']['import'] = array(
        '#type' => 'textarea',
        '#title' => t('field', '你也可以导入字段'),
        '#description' => t('field', '请将导出的代码复制到文本框，务必保证代码的完整性，不要做任何修改。可同时导入多个字段，注意：多个字段将直接保存，若遇冲突将跳过，不建议使用')
      );
    }

    $form['fields']['type'] = array(
      '#type' => 'hidden',
      '#default_value' => $field->type,
      '#constant' => 1,
    );

    $form['fields']['module'] = array(
      '#type' => 'hidden',
      '#default_value' => $field->module,
      '#constant' => 1,
    );

    $form['settings'] = array(
      '#validate' => array('field_form_select_validate'),
      '#redirect' => $field->redirect,
    );

    $form['fields']['submit']['#type'] = 'submit';
    return $form;
  }
  
  if (empty($options[$field->field_type])) {
    dd_set_message(t('field', '不允许的字段类型'), 'error');
    return;
  }

  if (!$field_type) {
    dd_set_message(t('field', '请求的字段类型不存在'), 'error');
    return;
  }
  
  $form['settings'] = array(
    '#validate' => array('field_form_validate'),
    '#description' => $group['description'],
    '#redirect' => $field->redirect,
    '#ajax_validate' => 1,
  );

  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('field', '名称'),
    '#weight' => -100,
    '#default_value' => $field->name,
    '#required' => 1,
  );

  $form['fields']['field_key'] = array(
    '#type' => 'textfield',
    '#title' => t('field', '索引'),
    '#weight' => -99,
    '#default_value' => $field->field_key,
    '#description' => t('field', '只能是数字、字母和下划线'),
    '#required' => 1,
  );
  
  $form['fields']['weight'] = array(
    '#default_value' => $field->weight ? $field->weight : 0,
    '#start' => -20,
    '#end' => 20,
    '#title' => t('field', '排序'),
    '#type' => 'weight'
  );
 
  $form['fields']['groups'] = array(
    '#type' => 'textfield',
    '#title' => t('field', '组名'),
    '#weight' => -99,
    '#default_value' => $field->groups,
    '#description' => t('field', '分组便于在输出表单时组织字段'),
  );
  
  $form['fields']['_field_type'] = array(
    '#title' => t('field', '类型'),
    '#value' => $field_type['title'],
  );
  
  if (empty($field->field_id)) {
    $form['fields']['_field_type']['#description'] = l(t('field', '返回重新选择'), $_GET['q'], array(
      'query' => dd_query_string_encode($_GET, array_merge(array('q', 'field_field_type'), array_keys($_COOKIE)))
    ));
  }
  
  $form['fields']['field_type'] = array(
    '#type' => 'hidden',
    '#constant' => 1,
    '#default_value' => $field->field_type,
  );

  $form['fields']['required'] = array(
  	'#type' => 'checkbox',
  	'#title' => t('field', '必需'),
  	'#default_value' => array($field->required),
  	'#options' => array(1 => t('field', '是'))
  );

  // 是否显示权限设置
  if (empty($group['disable_access'])) {
    $form['fields']['access'] = array(
      '#fieldset_prefix' => 'asc',
      '#fieldset_legend' => t('field', '权限'),
      '#fieldset_suffix' => 1,
      '#description' => t('field', '默认与节点权限相同，除非有特殊权限需求， 一般不需要设置以下选项'),
    );
    
    $roles = user_roles_opt();

    $form['fields']['access']['view'] = array(
      '#type' => 'checkbox',
      '#title' => t('field', '浏览'),
      '#default_value' => $field->access['view'],
      '#options' => $roles
    );

    $form['fields']['access']['update'] = array(
      '#type' => 'checkbox',
      '#title' => t('field', '填写'),
      '#default_value' => $field->access['update'],
      '#options' => $roles
    );
  }

  /*
  $form['fields']['data'] = array(
    '#fieldset_prefix' => 'asc',
    '#fieldset_legend' => t('field', '更多设置'),
    '#fieldset_suffix' => 1,
  );
  */
  $form['fields']['data']['input_description'] = array(
    '#type' => 'textfield',
    '#title' => t('field', '输入描述'),
    '#attributes' => array('size' => 50),
    '#description' => t('field', '显示在表单字段旁，辅助用户输入'),
    '#default_value' => $field->data['input_description'],
  );

  $form['fields']['data']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('field', '输出描述'),
    '#description' => t('field', '通常在浏览页面显示'),
    '#default_value' => $field->data['description'],
  );
  /**
   * 触发 hook_field_call_form()
   * 模块可以在表单显示、验证、提交各环节自定义操作
   */
  module_alter_all('field_call_form', $field, $form);
  
  $form['fields']['type'] = array(
    '#type' => 'hidden',
    '#default_value' => $field->type,
    '#constant' => 1,
  );
  $form['fields']['module'] = array(
    '#type' => 'hidden',
    '#default_value' => $field->module,
    '#constant' => 1,
  );
  $form['fields']['field_id'] = array(
    '#type' => 'hidden',
    '#default_value' => $field->field_id,
    '#constant' => 1,
  );
  
  $form['fields']['submit']['#type'] = 'submit';
 
  return $form;
 
}

function _field_save(&$field) {
 
  $error = NULL;

  $function = $field->module . '_field_call_group';

  // 基本验证
  if (!function_exists($function) || !$info = $function($field->type)) {
    dd_set_message(t('field', '未定义的字段组'), 'error');
    $error['module']['#error'][] = t('field', '未定义的字段组');
  } else if (!$field->module) {
    $error['module']['#error'][] = t('field', '模块名称不能为空');
  } else if (!field_get_call_lists($field->field_type)) {
    dd_set_message(t('field', '未定义的字段类型'), 'error');
    $error['field_type']['#error'][] = t('field', '未定义的字段类型');
  } else if (!$field->field_type) {
    $error['field_type']['#error'][] = t('field', '字段类型不能为空');
  } else if (!$field->type) {
    $error['type']['#error'][] = t('field', '字段标识不能为空');
  } else if (!$field->field_key) {
    $error['field_key']['#error'][] = t('field', '索引不能为空');
  } else if (mb_strlen($field->groups, 'utf-8') > 32) {
    $error['groups']['#error'][] = t('field', '字段分组长度不超过 %strlen 个字符', array('%strlen' => 32));
  } else if (mb_strlen($field->type, 'utf-8') > 32) {
    $error['type']['#error'][] = t('field', '字段标识长度不超过 %strlen 个字符', array('%strlen' => 32));
  } else if (mb_strlen($field->field_func, 'utf-8') > 64) {
    $error['field_func']['#error'][] = t('field', '回调函数长度不超过 %strlen 个字符', array('%strlen' => 64));
  } else if ($field->name && mb_strlen($field->name, 'utf-8') > 64) {
    $error['name']['#error'][] = t('field', '字符名称长度不超过 %strlen 个字符', array('%strlen' => 64));
  } else if (preg_match('/[^0-9|a-z|_|-]/i', $field->type)) {
    $error['type']['#error'][] = t('field', '字段标识只能是数字、字母与下划线');
  } else if ($field->field_func && preg_match('/[^0-9|a-z|_|-]/i', $field->field_func)) {
    $error['field_func']['#error'][] = t('field', '回调函数只能是数字、字母与下划线');
  } else if (is_numeric($field->field_key)) {
    $error['field_key']['#error'][] = t('field', '索引不能全部是数字');
  } else if (preg_match('/[^0-9|a-z|_|-]/i', $field->field_key)) {
    $error['field_key']['#error'][] = t('field', '索引名称只能是数字、字母与下划线');
  } else if (($field->field_id && db_query('SELECT COUNT(*) FROM {fields} WHERE field_key = ? AND module = ?
    AND type = ? AND field_id != ?', array($field->field_key, $field->module, $field->type, $field->field_id),
    array('return' => 'column'))) || (!$field->field_id && db_query('SELECT COUNT(*) FROM {fields} WHERE field_key = ? AND module = ?
    AND type = ?', array($field->field_key, $field->module, $field->type), array('return' => 'column')))) {
    $error['field_key']['#error'][] = t('field', '%string 已经存在', array('%string' => $field->field_key));
  } else {
    
    if (empty($field->access)) $field->access = array();
    if (empty($field->weight)) $field->weight = 0;
    $field->required = empty($field->required) ? 0 : 1;
    if (!isset($field->deleted)) $field->deleted = 1;
    if (empty($field->locked)) $field->locked = 0;
    if (empty($field->field_func)) $field->field_func = '';
    
    // 触发 hook_field_call_validate()
    if (!$error = module_alter_all('field_call_validate', $field)) {
      // 保存字段数据
      
      if ($field->field_id) {
        // 更新
        $args = array('field_id');
      } else {
        $args = NULL;
        // 添加
        if ($info['sum']) {
          $count = db_query('SELECT COUNT(*) FROM {fields} WHERE module = ? AND type = ?',
          array($field->module, $field->type), array('return' => 'column'));
          if ($count + 1 > $info['sum']) {
            dd_set_message(t('field', '该组最多添加 %length 个字段', array('%length' => $info['sum'])), 'error');
            $error['type']['#error'][] = t('field', '该组最多添加 %length 个字段', array('%length' => $info['sum']));
            return $error;
          }
        }
      }
      
      if (db_write_record('fields', $field, $args)) {
        dd_set_message(t('field', '保存成功'));
      } else {
        dd_set_message(t('field', '保存失败'), 'error');
        return false;
      }
      
      /**
       * 触发 hook_field_call_save()
       */
      module_invoke_all('field_call_save', $field);
      
      field_get_cache_field(NULL, 1); // 更新缓存
      return false;
    }
  }

  return $error;
}

/**
 * 添加界面字段类型选择 或 字段导入
 */
function field_form_select_validate($form, $v) {
  if ($v['import']) {
    $data = trim($v['import']);
  }
  
  if ($get = dd_query_string_encode($_GET, array_merge(array('q', 'field_field_type'), array_keys($_COOKIE)))) {
    $get .= '&';
  }
  
  if (!$data) {
    $get .= 'field_field_type='.$v['field_type'];
    dd_goto($_GET['q'], $get);
  } else {
    $data = preg_replace('/###(.*?)###/ms', '', $data);
    if (strpos($data, '[end]') !== false) {
      if ($datas = explode('[end]', $data)) {
        $datas = array_filter($datas);
        $count = count($datas);
        foreach ($datas as $f) {
          $f = trim(str_replace('[start]', '', $f));
          $export = base64_decode($f);
          $field = unserialize($export);
          unset($field->field_id);
          if ($count == 1) {
            $_SESSION['field_import'] = $field;
            $get .= 'field_field_import=true&field_field_type='.$field->field_type;
            dd_goto($_GET['q'], $get);
          } else {
            $field->type = $v['type'];
            $field->module = $v['module'];
            field_save($field);
            if ($field->field_id) {
              dd_set_message($field->name.' 添加成功');
            } else {
              dd_set_message($field->name.' 添加失败', 'error');
            }
          }
        }
      }
    }
  }
}

function _field_groups($module, $type) {
  if ($fetch = db_query('SELECT field_id, groups FROM {fields} WHERE module = ? AND type = ?',
  array($module, $type))) {
    dd_set_help(t('field', '管理字段分组在表单中显示时的 fieldset 信息'));

    $datas = custom_get('field_groups_' . $module . $type);

    $form = array(
      'fields' => array('groups' => array()),
      'settings' => array(
        '#validate' => array('field_groups_validate'),
      ),
      'values' => array('module' => $module, 'type' => $type),
    );

    foreach ($fetch as $o) {
      if (empty($o->groups)) continue;

      if (empty($datas[$o->groups])) {
        $data = array('legend' => '', 'prefix' => '');
      } else {
        $data = $datas[$o->groups];
      }

      $form['fields']['groups'][$o->groups] = array(
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => $o->groups,
        '#fieldset_suffix' => 1,
        '#fieldset' => true       
      );

      $form['fields']['groups'][$o->groups]['legend'] = array(
        '#title' => t('field', '分组名称'),
        '#type' => 'textfield',
        '#default_value' => $data['legend'],
        '#description' => t('field', 'fieldset legend 值，留空则为不启用 fieldset'),
      );
 
      $form['fields']['groups'][$o->groups]['prefix'] = array(
        '#title' => t('field', '默认样式'),
        '#type' => 'radio',
        '#default_value' => $data['prefix'],
        '#options' => array('asc' => t('field', '展开'), 'desc' => '闭合'),
        '#description' => t('field', '若启用 fieldset，请选择默认样式'),
      );
    }

    $form['fields']['button'] = array(
      '#type' => 'submit'
    );

    return $form;
  } else {
    dd_set_help(system_no_content(t('field', '还没有分组')));
    return false;
  }
}

function field_groups_validate(&$form, &$v) {
  $key = 'field_groups_' . $form['values']['module'] . $form['values']['type'];
  custom_set($key, $v['groups']);
  dd_set_message(t('field', '保存成功'));
}

function _field_export($field_id = NULL, $module = NULL, $type = NULL) {
  if ($field_id) {
    if (is_array($field_id)) {
      foreach ($field_id as $id) {
        $fields[$id] = field_load($id);
      }
    } else {
      $fields[$field_id] = field_load($field_id);
    }
  }
  
  if ($module && $type) {
    $fields = field_get_cache_field_group($module, $type);
  } else if ($module) {
    $fields = field_get_cache_field_group($module);
  }
  
  if ($fields) {
    foreach ($fields as $field) {
      module_alter_all('field_call_export', $field);
      $output .= '###'.$field->name."###\n";
      $output .= '[start]'.base64_encode(serialize($field))."[end]\n";
    }

    return theme_get_element('dd_form_textarea', array(
      '#value' => $output,
      '#name' => 'field_export',
      '#attributes' => array('id' => 'field_export_content_view')
    ));
  }
}


