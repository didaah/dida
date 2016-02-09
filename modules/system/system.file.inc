<?php
// $Id$

/**
 * 文件浏览页面
 */
function _system_file_view($file) {
  global $user;

  dd_set_title(array($file->filename));

  $file->path = url('files/' . $file->fid . '/view');
  $file->filebody = filter_view($file->filebody);

  module_invoke_all('file_view', $file);

  return theme('file', $file);
}

/**
 * 文件上传处理
 * @TODO 将使用 editor 模块取代 system 提供的编辑器相关功能
 */
function system_file_upload() {
  switch ($_GET['op']) {
    case 'ckeditor': case 'swfupload': case 'markitup':
      if (!empty($_GET['name']) && $_FILES[$_GET['name']]) {
        $validate = array();
        
        if (!empty($_GET['image_size'])) {
          $validate['image']['size'] = $_GET['image_size'];
        } else if (!empty($_GET['type']) && $_GET['type'] == 'image') {
          $validate['image'] = true;
        }
        
        if (!empty($_GET['ext_type'])) {
          $validate['ext_type'] = $_GET['ext_type'];
        }
        
        if (!empty($_GET['ext_id']) && is_numeric($_GET['ext_id'])) {
          $validate['ext_id'] = $_GET['ext_id'];
        }
        
        if (!empty($_GET['ext_id2']) && is_numeric($_GET['ext_id2'])) {
          $validate['ext_id2'] = $_GET['ext_id2'];
        }
        
        if (isset($_GET['status']) && is_numeric($_GET['status'])) {
          $validate['status'] = $_GET['status'];
        }
        
        $file = file_save_upload($_FILES[$_GET['name']], $validate);
        
        if ($_GET['op'] == 'ckeditor') {
          if ($file->filepath) {
            $filepath = f($file->filepath);
          } else {
            $error = t('system', '上传失败');
          }
          echo _dd_js_print('window.parent.CKEDITOR.tools.callFunction('.$_GET['CKEditorFuncNum'].', "'.$filepath.'", "'.$error.'")');
        } else if ($_GET['op'] == 'swfupload') {
          if ($file->filepath) {
            echo f($file->filepath);
          } else {
            header("HTTP/1.1 404 Internal Server Error");
          }
        } else if ($_GET['op'] == 'markitup') {
          $json = array('error' => '');
          if ($file->filepath) {
            $json['filepath'] = f($file->filepath);
          } else {
            $json['error'] = t('system', '上传失败');
          }
          echo json_encode($json);
        }
      }
    exit;
    case 'ckeditorbrowser': case 'custombrowser': case 'markitupbrowser':
      if (user_access('file one view')) {
      	global $user;
      	
      	if ($_GET['type'] == 'image') {
      	  
      	  if ($fetch = pager_query("SELECT fid, filepath, filename FROM {files} WHERE uid = ? AND filemime LIKE ? ORDER BY fid DESC",
      	  20, 0, NULL, array($user->uid, 'image/%'))) {
        	  foreach ($fetch as $o) {
              $thumb = image_get_thumb($o->fid, $o->filepath, '85x85');
              $item[] = '<span class="files_view_editor_image">'.img($thumb) . '</span><span class="files_view_'.$_GET['op'].'_insert">'.l('插入小图', f($thumb), array('attributes' => array('title' => $o->filename, 'target' => '_blank', 'link' => f($o->filepath)))).l('插入大图', f($o->filepath), array('attributes' => array('title' => $o->filename, 'target' => '_blank'))).'</span>';
            }
      	  }
      	  $type = 'image';
      	  
      	} else if ($fetch = pager_query("SELECT fid, filepath, filename, filesize, timestamp FROM {files} WHERE uid = ? ORDER BY fid DESC",
      	20, 0, NULL, array($user->uid))){
      	  foreach ($fetch as $o) {
      	    $data = '<span class="files_view_'.$_GET['op'].'_insert">';
      	    $data .= dd_get_strong('上传时间').format_date($o->timestamp);
      	    $data .= dd_get_strong('文件名').$o->filename.'('.round($o->filesize/1024, 2).'KB)';
      	    $data .= l('插入文件', f($o->filepath), array('attributes' => array('title' => $o->filename, 'target' => '_blank', 'link' => f($o->filepath))));
            $data .= '</span>';
            
            $item[] = $data;
          }
          $type = 'file';
      	}
      	
      	if ($item) {
      	  
      		$output = theme('item_list', $item, NULL, 'ul', array('id' => 'files_view_editor_'.$type));
      		$output .= pager_view();
      		
        } else {
          $output = system_no_content();
        }
        
      	echo theme('print', $output);
      	exit;
      }
  }
}
