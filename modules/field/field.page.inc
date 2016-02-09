<?php
// $Id$

/**
 * 字段浏览页面
 */
function field_page($field) {
  dd_set_title(array($field->name));
  dd_set_breadcrumb(array(l($field->name, 'fields/'.$field->field_id)));
  $content = module_invoke($field->module, 'field_view', $field);
  return theme('field_node_list', $content, $field, 'field');
}

/**
 * select、radio、selects、checkbox、tags类型浏览页面
 */
 
function field_term_page($term) {
  dd_set_title(array($term->name, $term->field->name));
  dd_set_breadcrumb(array(l($term->field->name, 'fields/' . $term->field->field_id), $term->name));
  $content = module_invoke($term->field->module, 'field_term_view', $term);
  return theme('field_node_list', $content, $term, 'term');
}

/**
 * 分类 rss
 */
function field_term_feed($term) {
  return module_invoke($term->field->module, 'field_term_feed', $term);
}

/**
 * 自动补全、多级联动等数据请求址
 */
function  _field_system_tools_ajax($v, $g) {
  dd_set_header('Content-Type: text/plain; charset=utf-8');
  $json['error'] = 1;
  
  switch ($g['op']) {
    case 'select':
      if ($v['__default_value']) {
        if ($fetch = db_query('SELECT tid, name FROM {fields_term} WHERE field_id = ? AND pid = ? 
          ORDER BY weight ASC, tid ASC', array($g['fid'], $v['__default_value']))) {
          foreach ($fetch as $o) {
            $json['contents'][] = array('tid' => $o->tid, 'name' => $o->name);
          }
          $json['error'] = 0;
        }
      }
    break;

    case 'autotag': // 标签提示
      if ($field = field_load($g['fid'])) {
        if (!empty($field->data['validate']['autocomplete']) && !empty($v['value'])) {
          $json['error'] = 0;

          $value = $v['value'];
          $prefix = '';

          if (strpos($value, ',') !== false || strpos($value, '，') !== false) {
            $arr = array_unique(explode(',', str_replace('，', ',', $value)));
            if (count($arr) < 2) {
              $json['error'] = 1;
              break;
            }

            $value = trim(array_pop($arr));

            if (empty($value)) break;

            $prefix = implode(',', $arr) . ',';
          }
          
          $v = str_replace(array('[-]', '[--]'), array(',', '，'), $value);

          if ($fetch = db_query('SELECT tid, name FROM {fields_term} WHERE field_id = ? AND name LIKE ? 
          ORDER BY weight ASC, tid ASC', array($g['fid'], $v . '%'), array('limit' => 10))) {
            foreach ($fetch as $o) {
              $o->name = str_replace(array(',', '，'), array('[-]', '[--]'), $o->name);
              if (empty($arr) || !in_array($o->name, $arr)) {
                $json['contents'][] = $prefix . $o->name;
              }
            }
          }
        }
      }
    break;

    case 'auto': // 输入 ajax 验证
      if ($field = field_load($g['fid'])) {
        if (in_array($field->data['validate']['type'], array('custom_ok', 'custom_not'))) {
          $value = $field->data['validate']['settings'][$field->data['validate']['type']];
          if (!empty($value['values'])) {
            $json['error'] = 0;
            switch ($value['value_type']) {
              case 1: // 普通模式
                $json['contents'] = dd_line_to_array($value['values'], 1);
              break;
              case 3: // php 模式
                $field_value = $v['value'];
                $field_type = 'autocomplete';
                $json['contents'] = eval($value['values']);
            }
          }
        }
      }
  }
  
  return json_encode($json);
}
