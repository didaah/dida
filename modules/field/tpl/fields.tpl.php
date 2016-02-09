<?php
// $Id$

/**
 * @file 节点字段组默认输出模板
 * @param string $module
 * @param string $type
 *  当节点有多个字段组时，可能合并输出，该种情况下， $type 为空
 * @param object $node
 *  $node->fields 字段原始数据
 *  $node->field_view 已经过组织的字段数据
 */
?>
<div class="fields-node-view fields-node-<?php echo $module;?>-view fields-node-<?php echo $module;?>-<?php echo $node->nid;?>-view" id="fields-node-<?php echo $module?>-<?php echo $type?>-<?php echo $node->nid?>-view"><ul>
  <?php
    if (!empty($node->field_view)) {
      foreach ($node->field_view as $field_id => $data) {
        if (!is_array($data)) continue;
        if (!empty($data['#content'])) {
          // view_type 为 2 时
          echo '<li class="fields-node-view-list fields-node-view-list-' . $data['#field_type'] . ' fields-node-view-list-' . $data['#field_type'] . '-' . $data['#field_key'] . '">';
          echo '<strong class="fields-node-view-name">' . $data['#name'] . '：</strong>';
          echo '<span class="fields-node-view-content">' . $data['#content'] . '</span>';
          echo '</li>';
        } else {
          // view_type 为 0或1 时
          foreach ($data as $d) {
            if (!is_array($d)) continue;
            if (!empty($d['#content'])) {
              echo '<li class="fields-node-view-list fields-node-view-list-' . $d['#field_type'] . ' fields-node-view-list-' . $d['#field_type'] . '-' . $d['#field_key'] . '">';
              echo '<strong class="fields-node-view-name">' . $d['#name'] . '</strong>：';
              echo '<span class="fields-node-view-content">' . $d['#content'] . '</span>';
              echo '</li>';
            }
          }
        }
      }
    } 
  ?>
</ul></div>
