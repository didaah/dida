<?php
// $Id$

/**
 * @file field 表单中的字段数据保存验证
 */

/**
 * select
 */
function field_select_field_node_validate($field, $value, &$error, $form, $ac) {
  global $conf;
  // 多级联动时，ajax 加载进来的内容，form.inc 无法验证其合法性
  if ($value->fields[$field->field_key] && $conf['field_term_children'][$field->field_id] && 
  !db_query('SELECT COUNT(field_id) FROM {fields_term} WHERE field_id = ? AND tid = ?',
  array($field->field_id, $value->fields[$field->field_key]), array('return' => 'column'))) {
    $error['fields'][$field->field_key]['#error'][] = t('field', '%string 出现非法选择', array('%string' => $field->name));
    dd_set_message(t('field', '%string 出现非法选择', array('%string' => $field->name)), 'error');
  }
}

/**
 * selects
 */
function field_selects_field_node_validate($field, $value, &$error, $form, $ac) {
  global $conf;
  if (!empty($field->required) && !$value->fields[$field->field_key]) {
    $error = t('field', '%string 不能为空', array('%string' => $field->name));
    dd_set_message($error, 'error');
    $error['fields'][$field->field_key]['#error'][] = $error;
  } else if (!empty($value->fields[$field->field_key]) && $conf['field_term_children'][$field->field_id] 
  && db_query('SELECT COUNT(field_id) FROM {fields_term} WHERE field_id != ? AND tid 
  IN (' . db_get_placeholder($value->fields[$field->field_key]).')',
  array_merge(array($field->field_id), $value->fields[$field->field_key]), array('return' => 'column'))) {
    $error['fields'][$field->field_key]['#error'][] = t('field', '%string 出现非法选择', array('%string' => $field->name));
    dd_set_message(t('field', '%string 出现非法选择', array('%string' => $field->name)), 'error');
  }
}

/**
 * tags
 */
function field_tags_field_node_validate($field, $value, &$error, $form, $ac) {

  if ($tags = field_get_string_tags($value->fields[$field->field_key])) {
    foreach ($tags as $tag) {
      if (mb_strlen($tag) > 64) {
        $error['fields'][$field->field_key]['#error'][] = t('field', '%string 的长度不超过 %length 字符', array('%string' => $tag, '%length' => 64));
        return;
      }
    }
  }

  if (empty($field->data['validate']['type']) || empty($value->fields[$field->field_key])) return;
  
  $v = !empty($field->data['validate']['settings']) ? $field->data['validate']['settings'][$field->data['validate']['type']] : array();

  switch ($field->data['validate']['type']) {
    case 'custom_ok':
      $e = false;
      switch ($v['value_type']) {
        case 1: // 普通模式
          $values = dd_line_to_array($v['values'], 1);
          if (array_search($value->fields[$field->field_key], $values) === false) {
            $e = true;
          }
        break;
        case 2: // 正则模式
          if (!preg_match($v['values'], $value->fields[$field->field_key])) {
            $e = true;
          }
        break;
        case 3: // php 模式，返回 false 表示未通过验证，true 表示通过验证，array 允许值列表
          $field_value = $value->fields[$field->field_key];
          $_return = eval($v['values']);
          if (empty($_return) || (is_array($_return) && !in_array($field_value, $_return))) {
            $e = true;
          }
        break;
        default:
          $e = true;
      }
      if ($e) {
        $error['fields'][$field->field_key]['#error'][] = t('field', '!string 的值不被允许', array('!string' => $field->name));
      }
    break;
    case 'custom_not':
      switch ($v['value_type']) {
        case 1: // 普通模式
          $values = dd_line_to_array($v['values'], 1);
          if (array_search($value->fields[$field->field_key], $values) !== false) {
            $e = true;
          }
        break;
        case 2: // 正则模式
          if (preg_match($v['values'], $value->fields[$field->field_key])) {
            $e = true;
          }
        break;
        case 3: // php 模式，返回 false 表示未通过验证，true 表示通过验证，array 允许值列表
          $field_value = $value->fields[$field->field_key];
          $_return = eval($v['values']);
          if (empty($_return) || (is_array($_return) && in_array($field_value, $_return))) {
            $e = true;
          }
        break;
        default:
          $e = true;
      }
      if ($e) {
        $error['fields'][$field->field_key]['#error'][] = t('field', '!string 的值不被允许', array('!string' => $field->name));
      }
  }
}

/**
 * textfield
 */
function field_textfield_field_node_validate($field, $value, &$error, $form, $ac) {
  if (isset($value->fields[$field->field_key])) {
    $e = NULL;
    $strlen = mb_strlen($value->fields[$field->field_key]);

    if ($strlen > 255) {
      $e = t('field', '%string 的长度不超过 %length 字符', array('%string' => $field->name, '%length' => 255));
    } else if (!empty($field->data['validate']['maxsize']) && $strlen > $field->data['validate']['maxsize']) {
      $e = t('field', '%string 的长度不超过 %length 字符', array('%string' => $field->name, '%length' => $field->data['validate']['maxsize']));
    } else if (!empty($field->data['validate']['minsize']) && $strlen < $field->data['validate']['minsize']) {
      $e = t('field', '%string 的长度不小于 %length 字符', array('%string' => $field->name, '%length' => $field->data['validate']['minsize']));
    } else if (!empty($field->data['validate']['type'])) { 
      $showtime = 0;
      $v = !empty($field->data['validate']['settings']) ? $field->data['validate']['settings'][$field->data['validate']['type']] : array();
      switch ($field->data['validate']['type']) {
        case 'figure':
          if (preg_match('/[^0-9]/', $value->fields[$field->field_key])) {
            $e = t('field', '!string 只能是数字', array('!string' => $field->name));
          }
        break;
        case 'letter':
          if (preg_match('/[^a-z]/i', $value->field[$field->field_key])) {
            $e = t('field', '!string 只能是字母', array('!string' => $field->name));
          }
        break;
        case 'figure_letter':
          if (preg_match('/[^a-z0-9]/i', $value->field[$field->field_key])) {
            $e = t('field', '!string 只能是字母和数字', array('!string' => $field->name));
          }
        break;
        case 'mail':
          $e = form_vali_mail($value->fields[$field->field_key]);
        break;
        case 'date_time':
          $showtime = 1;
        case 'date':
          static $start_or_end;
          if (!empty($value->fields[$field->field_key])) {
            if (!$e = field_validate_date($value->fields[$field->field_key], $v['value_type'], $showtime)) {
              if ($v['start_or_end'] && $v['field_key']) {
                if (!isset($start_or_end[$v['field_key']])) {
                  $start_or_end[$field->field_key] = (float)preg_replace(array('/\s/', '/[^\w]/'), array('.', ''), $value->fields[$field->field_key]);
                } else if ($v['start_or_end'] == 'start') {
                  // 开始字段
                  if (intval($value->fields[$field->field_key]) >= $start_or_end[$v['field_key']]) {
                    $e = t('field', '开始日期不能大于结束日期');
                  }
                } else {
                  // 结束字段
                  if ((float)preg_replace(array('/\s/', '/[^\w]/'), array('.', ''), $value->fields[$field->field_key]) <= $start_or_end[$v['field_key']]) {
                    $e = t('field', '开始日期不能大于结束日期');
                  }
                }
              }
            }
          }
        break;
        case 'chinese':
          if (preg_match("/[^\x{4e00}-\x{9fa5}]+$/u", $value->fields[$field->field_key])) {
            $e = t('field', '!string 只能是中文', array('!string' => $field->name));
          }
        break;
        case 'custom_ok':
          $e = false;
          switch ($v['value_type']) {
            case 1: // 普通模式
              $values = dd_line_to_array($v['values'], 1);
              if (array_search($value->fields[$field->field_key], $values) === false) {
                $e = true;
              }
            break;
            case 2: // 正则模式
              if (!preg_match($v['values'], $value->fields[$field->field_key])) {
                $e = true;
              }
            break;
            case 3: // php 模式，返回 false 表示未通过验证，true 表示通过验证，array 允许值列表
              $field_value = $value->fields[$field->field_key];
              $_return = eval($v['values']);
              if (empty($_return) || (is_array($_return) && !in_array($field_value, $_return))) {
                $e = true;
              }
            break;
            default:
              $e = true;
          }
          if ($e) $e = t('field', '!string 的值不被允许', array('!string' => $field->name));
        break;
        case 'url':
          if (!preg_match('/https?:\/\/([a-z0-9]*?)\.(.*)/i', $value->fields[$field->field_key])) {
            $e = t('field', '请输入正确的网址，如：http://www.didaah.org/');
          }
        break;  
        case 'custom_not':
          switch ($v['value_type']) {
            case 1: // 普通模式
              $values = dd_line_to_array($v['values'], 1);
              if (array_search($value->fields[$field->field_key], $values) !== false) {
                $e = true;
              }
            break;
            case 2: // 正则模式
              if (preg_match($v['values'], $value->fields[$field->field_key])) {
                $e = true;
              }
            break;
            case 3: // php 模式，返回 false 表示未通过验证，true 表示通过验证，array 不允许值列表
              $field_value = $value->fields[$field->field_key];
              $_return = eval($v['values']);
              if (empty($_return) || (is_array($_return) && in_array($field_value, $_return))) {
                $e = true;
              }
            break;
            default:
              $e = true;
          }
          if ($e) $e = t('field', '!string 的值不被允许', array('!string' => $field->name));
        break;
        case 'url':
          if (!preg_match('/https?:\/\/([a-z0-9]*?)\.(.*)/i', $value->fields[$field->field_key])) {
            $e = t('field', '请输入正确的网址，如：http://www.didaah.org/');
          }
        break;  
        case 'custom_not':
          switch ($v['value_type']) {
            case 1: // 普通模式
              $values = dd_line_to_array($v['values'], 1);
              if (array_search($value->fields[$field->field_key], $values) !== false) {
                $e = true;
              }
            break;
            case 2: // 正则模式
              if (preg_match($v['values'], $value->fields[$field->field_key])) {
                $e = true;
              }
            break;
            case 3: // php 模式
              $field_value = $value->fields[$field->field_key];
              if (eval($v['values']) == false) {
                $e = true;
              }
            break;
            default:
              $e = true;
          }
          if ($e) $e = t('field', '!string 的值不被允许', array('!string' => $field->name));
      }
    }

    if (empty($e) && !empty($field->data['validate']['unique'])) {
      if (!empty($value->nid)) {
        if (db_query('SELECT field_id FROM {fields_small} WHERE nid != ? AND field_id = ? AND text = ?',
        array($value->nid, $field->field_id, $value->fields[$field->field_key]), array('return' => 'column'))) {
          $e = t('field', '%string 已经存在', array('%string' => $field->name));
        }
      } else if (db_query('SELECT field_id FROM {fields_small} WHERE field_id = ? AND text = ?',
      array($field->field_id, $value->fields[$field->field_key]), array('return' => 'column'))) {
        $e = t('field', '%string 已经存在', array('%string' => $field->name));
      }
    }
   
    if ($e) $error['fields'][$field->field_key]['#error'][] = $e;
  }
}

/**
 * textarea
 */
function field_textarea_field_node_validate($field, $value, &$error, $form, $ac) {
  if ($value->fields[$field->field_key]) {
    $strlen = mb_strlen($value->fields[$field->field_key]);
    if (!empty($field->data['validate']['maxsize']) && $strlen > $field->data['validate']['maxsize']) {
      $error['fields'][$field->field_key]['#error'][] = t('field', '%string 的长度不超过 %length 字符',
        array('%string' => $field->name, '%length' => $field->data['validate']['maxsize']));
    } else if (!empty($field->data['validate']['minsize']) && $strlen < $field->data['validate']['minsize']) {
      $error['fields'][$field->field_key]['#error'][] = t('field', '%string 的长度不小于 %length 字符',
        array('%string' => $field->name, '%length' => $field->data['validate']['minsize']));
    }
  }
} 
