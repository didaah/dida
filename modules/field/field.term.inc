<?php
// $Id$
 
/**
 * @file term 相关的代码
 * 分类字段为：tags、select、radio、selects、checkbox
 * 与分类操作相关的接口：
 * 载入分类：hook_field_call_term_load(&$term)，$field 在 $term->field
 * 分类表单：hook_field_call_term_form(&$form, &$field, &$term)
 * 分类验证：hook_field_call_term_validate($field, $term)
 * 分类保存：hook_field_call_term_save($field, $term)
 * 分类删除：hook_field_call_term_delete($term)
 */

/**
 * 获取分类允许的最大层级
 * @param (array)$group
 * @param (object)$field
 */
function field_group_info_level($group, $field) {
  if ($group['children'] && $group['children'][$field->field_key] && $group['children'][$field->field_key]['level']) {
    return $group['children'][$field->field_key]['level'];
  } else {
    return $group['level'];
  }
}

/**
 * 验证层级分类层级，最多不超过 10 级
 * 检查 {tid} == $pid 的分类是否允许子分类
 */
function field_term_is_level($pid, $level) {
  if ($level == 1 && $pid) {
    return false;
  }
  
  if (!$pid) {
    return true;
  }
  
  if (!$level || $level > 10) {
    $level = 10;
  }
  
  static $lists;
  if (!isset($lists[$pid])) {
		$i = 2;
    $lists[$pid] = true;
    $_pid = $pid;
		while ($_pid = db_query('SELECT pid FROM {fields_term} WHERE tid = ? AND pid != 0', array($_pid), array('return' => 'column'))) {
			if ($level == $i) {
        $lists[$pid] = false;
        return false;
      }
      ++$i;
		}
  }
  return $lists[$pid];
}

/**
 * 按 pid 生成树形选择项
 */
function field_term_options($field, $level = NULL, $data = 0) {
  if (!$level || $level > 10) $level = 10;
  
  $key = 'field_cache_options_'.$field->field_id.'_'.$level.'_'.$data;
  
  if ($cache = cache_get($key, 'cache_field')) {
    return $cache->data;
  }
  
  $options[0] = $field->name;
  if ($fetch = db_query('SELECT pid, tid, name FROM {fields_term} WHERE field_id = ? ORDER BY pid ASC, weight ASC, tid ASC', array($field->field_id))) {
    foreach ($fetch as $o) {
      $datas[$o->pid][$o->tid] = $o;
    }
    
    foreach ($datas[0] as $o) {
      if (!$data) {
        if ($j > $level) break;
        $options[$o->tid] = '--'.$o->name;
      } else {
        $o->name = '--'.$o->name;
        $options[$o->tid] = $o;
      }
      
      if ($datas[$o->tid]) {
        _field_term_options($o->tid, $datas, $options, $data);
      }
    }
  }
  
  cache_set($key, $options, 'cache_field');
  
  return $options;
}

function _field_term_options($tid, $datas, &$options, $data, $i = '--') {
  $i .= '--';
  foreach ($datas[$tid] as $o) {
    $o->name = $i.$o->name;
    if (!$data) {
      $options[$o->tid] = $o->name;
    } else {
      $options[$o->tid] = $o;
    }
    
    if ($datas[$o->tid]) {
      _field_term_options($o->tid, $datas, $options, $data, $i);
    }
  }
}

/**
 * 载入分类
 */
function field_term_load($tid) {
	static $terms;
	if (!isset($term[$tid])) {
		if ($term = db_query('SELECT * FROM {fields_term} WHERE tid = ?', array($tid), array('return' => 'one'))) {
		  $field = field_load($term->field_id);
      $term->field = $field;
		  module_alter_all('field_call_term_load', $field, $term);
		}
		$terms[$tid] = $term;
	}
	return $terms[$tid];
}

/**
 * 验证分类浏览权限，权限继承自field
 */
function field_term_access($term) {
  return field_access('view', $term->field);
}

/**
 * 删除分类，同时将删除所有子分类，及 field_node 表中相关数据
 */
function field_term_delete($term) {
  if (is_numeric($term)) {
    $term = field_term_load($term);
  } else if (is_object($term)) {
    
    if (!$term->field_id) {
      $term = field_term_load($term->tid);
    }
    
  } else {
    return false;
  }
  
  if ($fetch = db_query('SELECT tid FROM {fields_term} WHERE field_id = ? AND pid = ?',
    array($term->field_id, $term->tid))) {
    foreach ($fetch as $o) {
      $sub = field_term_load($o->tid);
      if (db_exec('DELETE FROM {fields_term} WHERE tid = ?', array($o->tid))) {
        module_invoke_all('field_call_term_delete', $sub);
      }
    }
  }
  
  if (db_exec('DELETE FROM {fields_term} WHERE tid = ?', array($term->tid))) {
    module_invoke_all('field_call_term_delete', $term);
    cache_del('*', 'field_cache_options_', 'cache_field');
    field_get_cache_field(NULL, 1);
    field_set_cache_term($term->field->module, $term->field->type);
    return true;
  }
}

/**
 * 批量添加
 */
function field_term_import($field = NULL, $term = NULL) {
  if (!$field && $term->field_id) {
    $field = field_load($term->field_id);
  }
  if (!$field->field_id) {
    dd_set_message(t('field', '错误的请求'), 'error');
    return false;
  }
  
  // 读取字段组信息
  if (!$group = module_invoke($field->module, 'field_call_group', $field->type)) {
    dd_set_message(t('field', '错误的请求'), 'error');
    return false;
  }
  
  if ($_GET['field_term_pid'] && !$term->pid) {
    $term->pid = $_GET['field_term_pid'];
  }
  
  $sub_tabs[] = l(t('field', '返回管理'), $_GET['q']);
  if ($term->pid) {
    if ($get = dd_query_string_encode($_GET, array_merge(array('q', 'field_custom', 'field_term_tid', 'field_term_pid', 'redirect'), array_keys($_COOKIE)))) {
      $get .= '&field_custom=view&';
    }
    $get .= 'field_term_tid='.$term->pid;
    $sub_tabs[] = l(t('field', '返回上级'), $_GET['q'], array('query' => $get));
  }
  
  dd_set_sub_tabs($sub_tabs);
  
  $form['settings'] = array(
    '#validate' => array('field_term_import_validate'),
    '#ajax_validate' => 1,
  );
  
  $form['fields']['pid'] = array(
    '#title' => t('field', '上级'),
    '#type' => 'select',
    '#default_value' => $term->pid,
    '#options' => field_term_options($field, field_group_info_level($group, $field))
  );
  
  $form['fields']['field_id'] = array(
    '#type' => 'hidden',
    '#default_value' => $field->field_id,
    '#constant' => 1
  );
  
  $form['fields']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('field', '文本'),
    '#description' => t('field', '每行一条记录，格式为：名称|排序值|描述，名称必须，排序值和描述可选。如：电脑|10|电脑的描述'),
  );
  
  $form['fields']['submit']['#type'] = 'submit';
  return $form;
}


function field_term_import_validate($form, $v) {
  if ($field = field_load($v['field_id'])) {
    if ($array = dd_line_to_array($v['body'], true)) {
      foreach ($array as $term) {
        $term = trim($term);
        if (strpos($term, '|') !== false) {
          $t = explode('|', $term);
          $data[] = array('name' => $t[0], 'weight' => $t[1], 'description' => $t[2]);
        } else {
          $data[] = $term;
        }
      }
      field_term_batch_save($field, $v['pid'], $data);
      field_get_cache_field(NULL, 1);
      field_set_cache_term();
    }
  } else {
    dd_set_message(t('field', '不正确的字段'));
    return true;
  }
}

/**
 * 分类表单
 */
function field_term_form($field = NULL, $term = NULL) {
  if (!$field && $term->field_id) {
    $field = field_load($term->field_id);
  }
	if (!$field->field_id) {
		dd_set_message(t('field', '错误的请求'), 'error');
		return false;
	}
  
	// 读取字段组信息
	if (!$group = module_invoke($field->module, 'field_call_group', $field->type)) {
    dd_set_message(t('field', '错误的请求'), 'error');
    return false;
  }
  
  if ($_GET['field_term_pid'] && !$term->pid) {
    $term->pid = $_GET['field_term_pid'];
  }
  
  $sub_tabs[] = l(t('field', '返回管理'), $_GET['q']);
  if ($term->pid) {
    if ($get = dd_query_string_encode($_GET, array_merge(array('q', 'field_custom', 'field_term_tid', 'field_term_pid', 'redirect'), array_keys($_COOKIE)))) {
      $get .= '&field_custom=view&';
    }
    $get .= 'field_term_tid='.$term->pid;
    $sub_tabs[] = l(t('field', '返回上级'), $_GET['q'], array('query' => $get));
  }
  dd_set_sub_tabs($sub_tabs);
  
  $form['settings'] = array(
    '#validate' => array('field_term_form_validate'),
    '#ajax_validate' => 1,
  );
  
  $form['fields']['pid'] = array(
    '#title' => t('field', '上级'),
    '#type' => 'select',
    '#weight' => -100,
    '#default_value' => $term->pid,
    '#options' => field_term_options($field, field_group_info_level($group, $field))
  );
  
  $form['fields']['field_id'] = array(
  	'#type' => 'hidden',
  	'#default_value' => $field->field_id,
  	'#constant' => 1
  );
  
  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('field', '名称'),
    '#weight' => -90,
    '#default_value' => $term->name,
    '#required' => 1,
  );

  $form['fields']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('field', '描述'),
    '#default_value' => $term->description,
  );

  $form['fields']['tid'] = array(
    '#type' => 'hidden',
    '#default_value' => $term->tid,
    '#constant' => 1,
  );

  $form['fields']['weight'] = array(
    '#default_value' => $term->weight ? $term->weight : 0,
    '#start' => 20,
    '#end' => -20,
    '#weight' => -80,
    '#title' => t('field', '排序'),
    '#type' => 'weight'
  );
  
  module_alter_all('field_call_term_form', $form, $field, $term);
  
  $form['fields']['submit']['#type'] = 'submit';
  return $form;
}

/**
 * 分类验证、保存
 */
function field_term_form_validate($form, $v) {
  if ($field = field_load($v['field_id'])) {
    /**
     * 1、触发自定义验证 hook_field_call_term_validate()
     * 2、调用 field_term_save() 保存数据
     * 3、触发自定义保存 hook_field_call_term_save()
     */
    if ($term = field_term_save($field, $v['_dida_values'])) {
      field_get_cache_field(NULL, 1);
      field_set_cache_term();
      dd_set_message(t('field', '操作成功'));
    } else {
      return true;
    }
    
  } else {
    dd_set_message(t('field', '不正确的字段'));
    return true;
  }
}

/**
 * 保存分类数据，若存在则更新
 */
function field_term_save(&$field, $term, $batch = 0) {
  if (!isset($field->pid)) $field->pid = 0;

  if ($error = module_invoke_all('field_call_term_validate', $field, $term)) {
    dd_set_message(implode('<br />', dd_array2_to('value', $error)), 'error');
    return false;
  }

  if (mb_strlen($term->name, 'utf-8') > 64) {
		dd_set_message(t('field', '名称长度不超过 75 个字符'), 'error');
		return false;
  }

	if (!$group = module_invoke($field->module, 'field_call_group', $field->type)) {
    dd_set_message(t('field', '字段组不存在'), 'error');
    return false;
  }
  
	if (!field_term_is_level($term->pid, field_group_info_level($group, $field))) {
		dd_set_message(t('field', '层级深度已超过限制，添加失败'), 'error');
		return false;
	}
	
  if (!$term->field_id) $term->field_id = $field->field_id;
  
  if ($batch) {
    static $data;
    if (!isset($data[$term->field_id])) {
      $data[$term->field_id] = array($term->pid => array());
      if ($fetch = db_query('SELECT tid, pid, name FROM {fields_term} WHERE field_id = ?', array($term->field_id))) {
        foreach ($fetch as $o) {
          $data[$term->field_id][$o->pid][$o->tid] = $o->name;
        }
      }
    }
    if ($data[$term->field_id][$term->pid] && $tid = array_search($term->name, $data[$term->field_id][$term->pid])) {
      $term->tid = $tid;
      $newterm = field_term_update($term);
    } else {
      $newterm = field_term_insert($term);
    }

  } else if ($term->tid) {
    if (!db_query('SELECT tid FROM {fields_term} WHERE field_id = ? AND name = ? AND tid != ? AND pid = ?',
    array($field->field_id, $term->name, $term->tid, $term->pid), array('return' => 'column'))) {
      $newterm = field_term_update($term);
    } else {
      dd_set_message(t('field', '%string 已经存在', array('%string' => $term->name)), 'error');
      return false;
    }
  } else if (!db_query('SELECT tid FROM {fields_term} WHERE field_id = ? AND name = ? AND pid = ?',
  array($field->field_id, $term->name, $term->pid), array('return' => 'column'))) {
    $newterm = field_term_insert($term);
  } else {
    dd_set_message(t('field', '%string 已经存在', array('%string' => $term->name)), 'error');
    return false;
  }
  
  if ($newterm->tid) {
    module_invoke_all('field_call_term_save', $field, $newterm);
    cache_del('*', 'field_cache_options_', 'cache_field');
    return $newterm;
  }
}

/**
 * 写入分类数据
 */
function field_term_insert($term) {
  if (!$term->weight) $term->weight = 0;
  if (!$term->pid) $term->pid = 0;
  if (!$term->description) $term->description = '';
  $term->_op = 'insert';
  if (db_exec('INSERT INTO {fields_term} (pid, field_id, weight, name, description) VALUES (?, ?, ?, ?, ?)', array($term->pid, $term->field_id, $term->weight, $term->name, $term->description))) {
    if ($term->tid = db_last_insert_id()) return $term;
  }
}

/**
 * 更新分类数据
 */
function field_term_update($term) {
  if (!$term->weight) $term->weight = 0;
  if (!$term->pid) $term->pid = 0;
  if (!$term->description) $term->description = '';
  $term->_op = 'update';
  if (db_exec('UPDATE {fields_term} SET pid = ?, weight = ?, name = ?, description = ? WHERE tid = ?',
  array($term->pid, $term->weight, $term->name, $term->description, $term->tid), array('return' => 1))) {
    return $term;
  }
}

/**
 * 批量写入传统型分类字段(即 field_get_term_type())
 */
function field_term_batch_save(&$field, $pid = 0, $data = array(), $message = 1) {
  if (!$data) return false;
  foreach ($data as $term) {
    if (is_array($term)) {
      if ($term['name']) {
        $term['pid'] = $pid;
        if ($t = field_term_save($field, (object)$term, 1)) {
          if ($message) dd_set_message(t('field', '%string 添加成功', array('%string' => $t->name)));
          if ($term['childrens'] && is_array($term['childrens'])) {
            field_term_batch_save($field, $t->tid, $term['childrens'], $message);
          }
        }
      }
    } else if ($t = field_term_save($field, (object)array('pid' => $pid, 'name' => $term), 1)) {
      if ($message) dd_set_message(t('field', '%string 添加成功', array('%string' => $term)));
    }
  }
  return true;
}
