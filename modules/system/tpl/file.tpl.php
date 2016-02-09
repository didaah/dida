<?php
// $Id$

/**
 * @file 文件浏览默认模板
 * @param object $file
 *
 * 模板文件加载优先级
 *  file_{$file->ext_type}_{preg_replace('/^[0-9a-z]/', '', $file->filemime)}.tpl.php
 *  file_{$file->ext_type}.tpl.php
 *  file.tpl.php
 * 注意：模板文件命名时，filemime 只保留字母和数字，比如：image/png，则模板文件为 file_{$file->ext_type}_imagepng.tpl.php
 */
?>

<div class="file_view file_view_<?php echo $file->ext_type; ?>">
  <h2 class="filename"><?php echo $file->filename; ?></h2>
  <div class="filebody"><?php echo $file->filebody;?></div>
  <div class="fileinfo">@<?php echo theme('username', $file); ?> <?php echo format_date($file->timestamp); ?></div>
  <div class="fileview">
    <?php if (strpos($file->filemime, 'image/') !== false) {  ?>
      <?php echo img($file->filepath, $file->filename, $file->filename); ?>
    <?php } else {?>
      <?php echo t('system', '下载文件：!name(!size M)', array(
                   '!name' => l(basename($file->filepath), $file->filepath),
                   '!size' => round($file->filesize/1048576, 3))
                 ); 
      ?>
    <?php } ?>
  </div>
</div>
