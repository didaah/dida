<?php
// $Id$

/**
 * 字段所属节点列表浏览页
 * @param string $content 输出的内容
 * @param object $data 若为一个类别，则为该类别对象($term)，若为所有类别，则为字段对象($field)
 * @param string $type 当前列表类型，term：指定类别下的所有节点，field：该字段中所有类别下的所有节点
 *
 * @param object $field 字段对象
 */

?>
<div class="field_node_list field_node_list_<?php echo $type; ?> field_node_list_<?php echo $type;?>_<?php echo $field->module?> field_node_list_<?php echo $type;?>_<?php echo $field->module?>_<?php echo $field->type?> field_node_list_<?php echo $type;?>_<?php echo $field->module?>_<?php echo $field->type?>_<?php echo $field->field_key;?>">
  <?php if (!empty($content)) : ?><div class="field_content_node_list"><?php echo $content?></div> <?php endif; ?>
</div>
