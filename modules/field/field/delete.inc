<?php
// $Id$

/**
 * @file 节点删除，需删除字段数据
 * 触发 hook_TYPE_field_node_delete() 前，将执行 module_include_file('field/delete.inc');
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/delete.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/delete.inc')
 */

/**
 * radio
 */
function field_radio_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid IN 
  (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($node->nid, $field->field_id));
}

/**
 * select
 */
function field_select_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid IN 
  (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($node->nid, $field->field_id));
}

/**
 * selects
 */
function field_selects_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid IN 
  (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($node->nid, $field->field_id));
}

/**
 * checkbox
 */
function field_checkbox_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid IN 
  (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($node->nid, $field->field_id));
}

/**
 * tags
 */
function field_tags_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid IN 
  (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($node->nid, $field->field_id));
}

/**
 * textfield
 */
function field_textfield_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_small} WHERE nid = ? AND field_id = ?',
  array($node->nid, $field->field_id));
}

/**
 * textarea
 */
function field_textarea_field_node_delete($field, $node) {
  db_exec('DELETE FROM {fields_long} WHERE nid = ? AND field_id = ?',
  array($node->nid, $field->field_id));
}

/**
 * file
 */
function field_file_field_node_delete($field, $node) {
  if ($files = _field_get_node_data($node->nid, 'file', 1)) {
    foreach ($files as $file) {
      if ($file->field_id == $field->field_id) {
        file_delete($file->fid, 1, $file->filepath);
      }
    }
  }
}
