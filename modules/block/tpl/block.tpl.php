<?php
// $Id$

/**
 * @file 区块默认模板
 * @param $block
 * 将按以下顺序加载模板文件：
 * block_{$block->module}_{$block->delta}.tpl.php
 * block_{$block->module}.tpl.php
 * block.tpl.php
 */
 
?>

<div class="block block_module_<?php echo $block->module ?>" id="block_<?php echo $block->module ?>_<?php echo $block->delta ?>">
  <?php if ($block->title) :?>
    <h3 class="block_title"><?php echo $block->title?></h3>
  <?php endif?>
  <div class="block_content">
    <?php echo $block->content?>
  </div>
</div>
