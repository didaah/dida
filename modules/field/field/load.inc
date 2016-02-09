<?php
// $Id$

/**
 * @file 节点加载时附加字段数据
 * 触发 hook_TYPE_field_node_load() 前，将执行 module_include_file('field/load.inc');
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/load.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/load.inc')
 */

/**
 * radio
 */
function field_radio_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'term', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key] = $o;
      }
    }
  }
}

/**
 * select
 */
function field_select_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'term', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key] = $o;
      }
    }
  }
}

/**
 * selects
 */
function field_selects_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'term', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key][$o->tid] = $o;
      }
    }
  }
}

/**
 * checkbox
 */
function field_checkbox_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'term', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key][$o->tid] = $o;
      }
    }
  }
}

/**
 * tags
 */
function field_tags_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'term', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key][$o->tid] = $o;
      }
    }
  }
}

/**
 * textfield
 */
function field_textfield_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, $field->field_type, 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key] = $o;
        continue;
      }
    }
  }
}

/**
 * textarea
 */
function field_textarea_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, $field->field_type, 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key] = $o;
        continue;
      }
    }
  }
}

/**
 * file
 */
function field_file_field_node_load($field, $node, $ac = NULL) {
  if ($fetch = _field_get_node_data($node->nid, 'file', 1)) {
    foreach ($fetch as $o) {
      if ($o->field_id == $field->field_id) {
        $node->fields[$field->field_key][$o->fid] = $o;
      }
    }
  }
}

/**
 * 按类型获取节点下所有字段数据
 * @param int $nid
 *  节点 id
 * @param string $type
 *  term：分类，textfield：单行文本，textarea：多行文本，file：上传字段
 * @param bool $clear
 *  是否直接从数据库查询
 *
 * 本函数获取的 nid 数据，包括所有对应的字段，请不要直接通过本函数来获取节点对应的字段数据
 */
function _field_get_node_data($nid, $type = 'term', $clear = NULL) {
  static $data;
  
  if ($clear) {
    $data[$nid] = NULL;
  }

  if (!isset($data[$nid])) $data[$nid] = array();
  if (!isset($data[$nid][$type])) {
    switch ($type) {
      case 'term':
        $data[$nid][$type] = db_query('SELECT t.field_id, t.pid, t.name, t.tid, t.weight FROM {fields_term_node} n INNER JOIN {fields_term} t ON n.tid = t.tid WHERE n.nid = ?', array($nid));
      break;
      case 'file':
        // ext_id：字段id，ext_id2：节点id
        $data[$nid][$type] = db_query('SELECT fid, ext_id AS field_id, filepath, filename, filesize, filemime, timestamp, filebody FROM {files} WHERE ext_id2 = ? AND ext_type = ?', array($nid, 'field_file'));
      break;
      case 'textfield':
        $data[$nid][$type] = db_query('SELECT text, field_id FROM {fields_small} WHERE nid = ?', array($nid));
      break;
      case 'textarea':
        $data[$nid][$type] = db_query('SELECT text, field_id FROM {fields_long} WHERE nid = ?', array($nid));
    }
  }
  return $data[$nid][$type];
}

