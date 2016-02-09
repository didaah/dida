<?php
// $Id$
/*
  '#enctype' => 'multipart/form-data'
  $form['fields']['file'] = array(
    '#type' => 'file',
    '#title' => '文件上传',
    '#multi' => 5,// (int) 多文件
    '#call_function' => '',// 回调自定义函数处理文件，此选项为真，以下选项无效。
    '#filesave' => true,// 系统处理上传文件
    '#validate' => array(
      'savepath' => '',// 存储路径，留空则使用默认路径。
      //'no_insert' => true, // 为 true 则文件信息不写入数据库
      //'title' => '', // 文件名或标题，不大于 255 字符
      //'filename' => '', // 路径中的文件名，可包含扩展名，若不指定，则使用 $_FILES['name']
      //'extension' => '', // 也可单独指定扩展名
      'ext_id' => 0, // 扩展 Id
      'ext_id2' => 0, // 扩展 Id
      'ext_type' => '', // 扩展类型
      'status' => 1, // 文件状态，默认为 1，即正常
      'image' => array( // 验证图片
        'type' => array('jpg', 'png', 'gif'),
        'size' => '200x300',
      ),
      'filetype' => array('jpg', 'zip'),// 允许的文件类型
      'filesize' => '300',// 允许的最大文件
      'usersize' => true, // 验证用户上传文件总大小
      'call_function' => '',// 验证完成，并保存后，回调函数，call_function($form_id, $field_name, $file, $form_token)
    ),
  );
*/

/**
function file_test() {
  $form['settings']['#attributes']['enctype'] = 'multipart/form-data';
  $form['fields']['file'] = array(
    '#type' => 'file',
    '#title' => '文件上传',
    '#multi' => 5,
    '#filesave' => true,
    '#required' => true,
    '#validate' => array(
      'savepath' => date('Y') .'/' .date('m'),
      'image' => array(
        'type' => array('jpg', 'png', 'gif'),
        'size' => '700x600',
      ),
      'filesize' => '4000',
      'usersize' => true,
    ),
  );
  $form['fields']['submit'] = array('#value' => '提交', '#type' => 'submit');
  return $form;
}
*/

/**
 * 上传字段 HTML
 */
function file_get_form($field) {
  $name = str_replace(array('[', ']'), array('_'), $field['#name']);
  $output = '';
  if ($field['#required'] && strpos($field['#attributes']['class'], 'required') === false) {
    $field['#attributes']['class'] .= ' required';
  }
  if ($field['#multi']) {
    static $dom = 1;
    dd_add_js('misc/jquery.MultiFile.js');
    
    if ($field['#validate']['image']['type']) {
      $type = implode('|', $field['#validate']['image']['type']);
    } else if ($field['#validate']['filetype']) {
      $type = implode('|', $field['#validate']['filetype']);
    } else if ($field['#filetype']) {
      $type = implode('|', $field['#filetype']);
    }
    
    $output .= '<input id="multi_'.$dom.'" class="multi" filelength="'.$field['#multi'].'" accept="'.$type.'" type="file" name="'.$field['#name'].'[]"'.dd_attributes($field['#attributes']).'/>';
    $output .= '<div id="multi_list_'.$dom.'"></div>';
    dd_add_js(array('multi' => array(array('dom' => $dom))), 'setting');
    
    ++$dom;
  } else {
    $output .= '<input type="file" name="'.$field['#name'].'" value="" '.dd_attributes($field['#attributes']).'/>';
  }
  return $output;
}

/**
 * 读取数据库，并显示图片
 */
function file_image_view($fid, $opt = array()) {
  if ($file = db_query('SELECT filepath, filename FROM {files} WHERE fid = ?',
  array($fid), array('return' => 'one'))) {
    $title = $opt['title'] ? $opt['title'] : $file->filename;
    $alt = $opt['alt'] ? $opt['alt'] : $title;
    
    if (!empty($opt['thumb'])) {
      $file->filepath = image_get_thumb($fid, $file->filepath, $opt['thumb']);
    }
    
    return img($file->filepath, $alt, $title);
  }
}

/**
 * 验证上传字段
 * @param (array) $form
 *  表单
 * @param (bool) $save
 *  验证完成后是否保存
 */
function file_form_validate($form, $save = true) {
  if ($_FILES) {
    foreach ($_FILES as $key => $field) {
      if (!empty($field['name'])) {
        if ($form['fields'][$key]['#type'] == 'file') {
          if ($form['fields'][$key]['#required'] && !file_require_empty($field)) {
            $error[$key]['#error'][] = t('system', '必须上传文件');
          }
          $form['fields'][$key]['#filename'] = $key;
          if ($msg = _file_form_validate($field, $form['fields'][$key], $save)) {
            $error[$key]['#error'] = $msg;
          }
        } else if (is_array($field['name'])) {
          $error[$key] = _file_form_subvalidate($form['fields'][$key], $field, $save);
        }
      }
    }
  }
  if ($error) {
    return array_filter($error);
  }
}

/**
 * 检查上传字段
 * @param (array) $files
 *  文件对象
 * @param (array) $field
 *  表单字段
 * @param (bool) $save
 *  检查通过后是否保存
 */
function _file_form_validate($files, $field, $save = true) {
  if (!$field['#filesave']) return false;
  if ($field['#multi']) {
    for ($i = 0; $i < $field['#multi']; ++$i) {
      if (!$t = file_validate_error($files['error'][$i])) {
        $file[$i] = array(
          'name' => $files['name'][$i],
          'type' => $files['type'][$i],
          'size' => $files['size'][$i],
          'tmp_name' => $files['tmp_name'][$i],
        );
      } else if ($t !== UPLOAD_ERR_NO_FILE) { 
        return $t;
      }
    }
  } else if (!$t = file_validate_error($files['error'])) {
    $file[] = $files;
  } else if ($t !== UPLOAD_ERR_NO_FILE) {
    return $t;
  }
  
  if ($save) {
    global $form_tokens; 
    // 用户通过表单上传，form_token 才有值，而直接调用函数上传，可能为空
    $token = !empty($form_tokens[$field['#form_id']]) ? $form_tokens[$field['#form_id']] : '';
    if (!empty($field['#call_function'])) {
      $error = call_user_func($field['#call_function'],  $field['#form_id'],  $field['#name'], $file, $token);
    } else if ($field['#filesave']) {
      $error = file_form_save($field, $file);
    }
  }
  
  return $error;
}

/**
 * 检查子字段
 */
function _file_form_subvalidate($form, $files, $save = true) {
  foreach ($files['name'] as $key => $field) {
    
    $file = array(
      'name' => $files['name'][$key],
      'type' => $files['type'][$key],
      'size' => $files['size'][$key],
      'tmp_name' => $files['tmp_name'][$key],
      'error' => $files['error'][$key],
    );
    
    if (is_string($field)) {
      if ($form[$key]['#type'] == 'file') {
        if ($form[$key]['#required'] && !$field) {
          $error[$key]['#error'][] = t('system', '必须上传文件');
        }
        $form[$key]['#filename'] = $key;
        if ($t = _file_form_validate($file, $form[$key], $save)) {
          $error[$key]['#error'] = $t;
        }
      }
    } else if (is_array($field)) {
      if ($t = _file_form_subvalidate($form[$key], $file, $save)) {
        $error[$key] = $t;
      }
    }
  }
  return $error;
}

/**
 * 将 $_FILES 转换为二维数组，排除未上传字段
 */
function file_get_array_data() {
  $files = array();
  foreach ($_FILES as $key => $field) {
    if (is_string($field['name'])) {
      if ($field['error'] === UPLOAD_ERR_OK) {
        $files[$key] = $field;
      }
    } else if (is_array($field['name'])) {
      if ($r = _file_get_array_data($_FILES[$key], $key)) {
        $files = array_merge($files, $r);
      }
    }
  }
  
  return $files;
}

function _file_get_array_data($array, $keys = '') {
  if (!is_array($array) || !$array['name']) return false;
  
  foreach ($array['name'] as $key => $field) {
    
    $file = array(
      'name' => $array['name'][$key],
      'type' => $array['type'][$key],
      'size' => $array['size'][$key],
      'tmp_name' => $array['tmp_name'][$key],
      'error' => $array['error'][$key],
    );
    
    if ($keys) {
      $index = $keys . '_' . $key;
    } else {
      $index = $key;
    }
    
    if (is_string($field)) {
      if ($file['error'] === UPLOAD_ERR_OK) {
        $files[$index] = $file;
      }
    } else if (is_array($field)) {
      if ($r = _file_get_array_data($file, $index)) {
        $files = array_merge($files, $r);
      }
    }
  }
  
  return $files;
}

/**
 * 判断必需上传字段是否为空
 */
function file_require_empty($array) {
  if (is_array($array)) {
    foreach ($array as $key => $val) {
      if (is_array($val)) {
        return dd_array_empty($val);
      } else if (!empty($val)) {
        return true;
      }
    }
  }
}

/**
 * 处理上传文件，默认调用 file_save_upload() 保存
 * 若设置 $form['#validate']['call_function']，回调该函数，传递字段名及文件对象
 */
function file_form_save($form, $files) {
  if (is_array($files)) {
    foreach ($files as $item) {
      if ($file = file_save_upload($item, $form['#validate'])) {
        global $form_tokens; 
        // 用户通过表单上传，form_token 才有值，而直接调用函数上传，可能为空
        $token = !empty($form_tokens[$form['#form_id']]) ? $form_tokens[$form['#form_id']] : '';

        if (empty($form['#validate']['call_function'])) {
          dd_set_message(t('system', '%string 上传成功', array('%string' => $file->filename)));
        } else {
          call_user_func($form['#validate']['call_function'], $form['#form_id'], $form['#name'], $file, $token);
        }
      }
    }
  }
}

/**
 * 保存上传文件
 * @param (string) $file
 *  表单上传数组
 * @param (array) $validate
 *  配置参数，见上传示例
 * @param (int) $rename
 *  若同名文件存在，可选设置，默认为重命名
 */
function file_save_upload($file, $validate = NULL, $rename = 1) {
  if (isset($file['error']) && file_validate_error($file['error']) != false) return false;
  
  if (!$file['name'] || !$file['tmp_name']) return false;
  
  $file = (object) $file;
  $file->filename = $file->name;
  
  if (file_create_name($file, $validate, $rename) && move_uploaded_file($file->tmp_name, $file->filepath) && file_save_validate($file, $validate)) {
    /**
     * 上传图片文件时，验证中参数中指定了文件类型，但所上传的类型不匹配时，直接将图片格式转换
     */
    if (!empty($file->imagetotype) && count($file->imagetotype) == 2) {
      image_to_type_data($file->filepath, $file->imagetotype[0], $file->imagetotype[1], $file->filepath);
    }

    if (empty($validate['no_insert'])) {
      return file_save($file); // 文件信息写入数据库
    } else {
      return $file; // 不写入
    }
  }
}

/**
 * 复制文件
 * @param (string) $source
 *  原文件路径，必须可读
 * @param (array) $validate
 *  配置参数，见上传示例
 * @param (int) $rename
 *  若同名文件存在，可选设置，默认为重命名
 */
function file_save_copy($source, $validate = NULL, $rename = 1) {
  if (!is_file($source) || !is_readable($source)) {
    dd_set_message(t('system', '没有权限访问文件 %string', array('%string' => $source)));
    return false;
  }
  
  $file = new stdClass();
  $file->filename = basename($source);
  
  if (file_create_name($file, $validate, $rename) && copy($source, $file->filepath) && file_save_validate($file, $validate)) {
    if (!$validate['no_insert']) {
      return file_save($file); // 文件信息写入数据库
    } else {
      return $file; // 不写入
    }
  }
}

/**
 * 移动文件
 * @param (string) $source
 *  原文件路径，必须可读
 * @param (array) $validate
 *  配置参数，见上传示例
 * @param (int) $rename
 *  若同名文件存在，可选设置，默认为重命名
 */
function file_save_move($source, $validate = NULL, $rename = 1) {
  if (!is_file($source) || !is_writable($source)) {
    dd_set_message(t('system', '没有权限移动文件 %string', array('%string' => $source)));
    return false;
  }
  
  $file = new stdClass();
  $file->filename = basename($source);
  
  if (file_create_name($file, $validate, $rename) && rename($source, $file->filepath) && file_save_validate($file, $validate)) {
    if (!$validate['no_insert']) {
      return file_save($file); // 文件信息写入数据库
    } else {
      return $file; // 不写入
    }
  }
}

/**
 * 字符串保存为文件
 * @param (string) $data
 *  待保存的字符串
 * @param (array) $validate
 *  配置参数，见上传示例。必须有 filename 参数，即文件名
 * @param (int) $rename
 *  若同名文件存在，可选设置，默认为重命名
 */
function file_save_data($data, $validate = array(), $rename = 1) {
  if (!$data) return false;
  if ($validate['filename']) {
    $file = new stdClass();
    
    if (file_create_name($file, $validate, $rename)) {
      if (!$fp = @fopen($file->filepath, 'wb')) {
        dd_set_message(t('system', '无法写入文件 %string', array('%string' => $file->filepath)), 'debug');
        return false;
      }
      
      @fwrite($fp, $data);
      @fclose($fp);
      
      if (file_save_validate($file, $validate)) {
        if (!$validate['no_insert']) {
          return file_save($file); // 文件信息写入数据库
        } else {
          return $file; // 不写入
        }
      }
      file_delete_file($file->filepath);
    }
  } else {
    dd_set_message(t('system', '文件名不能为空'));
  }
}

/**
 * 保存前检查文件名
 * @param (object) $file
 *  文件对象
 * @param (array) $validate
 *  验证参数
 * @param (int) $rename
 *  若同名文件存在，可选设置，默认为重命名
 */
function file_create_name(&$file, $validate = NULL, $rename = 1) {
  if (!empty($validate['filepath'])) {
    $file->filepath = $validate['filepath'];
  } else if ($validate['savepath']) {
    $savepath = file_mkdir(file_directory_path() . '/' . $validate['savepath']);
  } else {
    $savepath = file_get_path();
  }
  
  if ($validate['filename']) {
    if (strpos($validate['filename'], '.') !== false) {
      $file->filename = $validate['filename'];
    } else {
      if (!empty($file->filename)) {
        $file->extension = strtolower(end(explode('.', $file->filename)));
        $file->filename = $validate['filename'] . '.' . $file->extension;
      } else {
        $file->filename = $validate['filename'];
      }
    }
  }
  
  if (!$file->filename) {
    dd_set_message(t('system', '不合法的文件名'), 'error');
    return false;
  }
  
  if (!empty($savepath) && !is_writable($savepath)) {
    dd_set_message(t('system', '文件夹 %string 必须有读取权限', array('%string' => $savepath)), 'error');
    return false;
  }
  
  if (empty($file->uid)) {
    $file->uid = $GLOBALS['user']->uid;
  }
  
  if (empty($file->extension)) {
    $_ext = explode('.', $file->filename);
    $file->extension = strtolower(end($_ext));
  }

  // 指定扩展名，若与上传类型不符，则转换，因此，只有图片才能指定扩展名
  if (!empty($validate['extension'])) {
    $validate['extension'] = strtolower($validate['extension']);
    if ($file->extension != $validate['extension'] && in_array($validate['extension'], array('jpg', 'jpeg', 'png', 'gif'))) {
      $file->imagetotype = array($file->extension, $validate['extension']);
      $file->extension = $validate['extension'];
      $file->filename .= '.' . $file->extension;
    }
  }
 
  if (!empty($validate['filetype']) && !file_validate_mime($file, $validate['filetype'])) {
    dd_set_message(t('system', '只允许 %string 类型的文件', array('%string' => implode(' ', $validate['filetype']))), 'error');
    return false;
  }
  
  if (empty($file->filepath)) {
    // 使用 md5 重命名文件
    if (var_get('file_upload_rename', 0) && empty($validate['filename'])) {
      $file->filepath = $savepath .'/'. md5($file->filename) . '.' . $file->extension;
    } else {
      $file->filepath = $savepath .'/'. $file->filename;
    }
  }
  
  // 重命名可执行的文件
  if (preg_match('/\.(php|pl|py|cgi|asp|js|html)$/i', $file->filename)) {
    $file->filemime = 'text/plain';
    $file->filepath .= '.txt';
    $file->filename .= '.txt';
    $file->extension = 'txt';
  }
  
  if (!$rename) {
    return true;
  }
  
  $oldfilepath = $file->filepath;
  
  //$rename: 0，替换。1，添加后缀。2，放弃
  
  if (is_file($file->filepath)) {
    $i = 0;
    switch ($rename) {
      case 1:
        $name = basename($oldfilepath);
        $file->filepath = str_replace($name, $file->uid.$i.'_'.$name, $oldfilepath);
        while (is_file($file->filepath)) {
          $file->filepath = str_replace($name, $file->uid.$i.'_'.$name, $oldfilepath);
          ++$i;
        }
      break;
      case 2:
        dd_set_message(t('system', '%string 已经存在', array('%string' => $file->filepath)));
      return false;
    }
  }
  
  return true;
}

/**
 * 验证已保存的文件
 */
function file_save_validate(&$file, $validate) {
  if (!$file->extension) {
    $file->extension = end(explode('.', $file->filepath));
  }
  if (!$file->filemime) {
    $file->filemime = file_get_mimetype($file->extension);
  }
  
  if (!$file->size) {
    $file->filesize = filesize($file->filepath);
  } else {
    $file->filesize = $file->size;
  }
  
  $file->status = isset($validate['status']) ? $validate['status']: 1;
  $file->filebody = isset($validate['filebody']) ? $validate['filebody']: '';
  
  if (is_array($validate)) {
    foreach ($validate as $key => $val) {
      switch ($key) {
        case 'ext_id':
          $file->ext_id = $val;
        break;
        case 'timestamp':
          $file->timestamp = $val;
        break;
        case 'filebody':
          $file->filebody = $val;
        break;
        case 'ext_id2':
          $file->ext_id2 = $val;
        break;
        case 'ext_type':
          $file->ext_type = $val;
        break;
        case 'title':
          $file->filename = $val;
        break;
        case 'image':
          if (!is_array($val)) $val = array();
          if (empty($val['type'])) $val['type'] = array('jpg', 'png', 'gif', 'jpeg', 'wbmp');
          if (!file_validate_image($file, $val)) {
            file_delete_file($file->filepath);
            return false;
          }
        break;
        case 'filesize':
          if (!file_validate_size($file, $val)) {
            file_delete_file($filepath);
            dd_set_message(t('system', '只允许 %string K 之内的文件', array('%string' => $val)), 'error');
            return false;
          }
        break;
        case 'filetype':
          if (is_array($val) && !file_validate_mime($file, $val)) {
            file_delete_file($filepath);
            dd_set_message(t('system', '只允许 %string 类型的文件。', array('%string' => implode(' ', $val))), 'error');
            return false;
          }
        break;
        case 'usersize':
          if ($size = file_validate_usersize($file)) {
            file_delete_file($filepath);
            dd_set_message(t('system', '文件空间不足。你的总空间 %sum MB，已使用 %used', array('%sum' => $size[0], '%used' => $size[1])), 'error');
            return false;
          }
        break;
        default:
          if (!isset($file->{$key})) {
            $file->{$key} = $val;
          }
      }
    }

    if ((!empty($validate['watermark']) || (!isset($validate['watermark']) 
      && var_get_key('file_image_watermark', 'default', 0))) 
      && strpos($file->filemime, 'image/') !== false) {
      image_gd_mark($file->filepath);
    }
    
  }
  
  return true;
}

/**
 * 文件信息写入数据库 
 * @param object $file 
 *  待写入的文件对象
 * @access public
 * @return bool|object 成功返回 $file，失败返回 false
 */
function file_save($file) {
  
  if (empty($file->filepath)) return false;

  if (!isset($file->uid)) $file->uid = $GLOBALS['user']->uid;
  
  if (empty($file->timestamp)) $file->timestamp = $_SERVER['REQUEST_TIME'];
  
  if (!empty($file->body)) $file->filebody = $file->body;

  if (empty($file->filemime)) {
    $file->filemime = file_get_mimetype(end(explode('.', $file->filepath)));
  }

  if (empty($file->filesize)) {
    $file->filesize = filesize($file->filepath);
  }

  if (empty($file->filename)) {
    $file->filename = end(explode('/', $file->filepath));
  }
 
  if (!empty($file->fid)) {
    $args[] = 'fid';
  } else {
    $args = NULL;
  }
  
  if (db_write_record('files', $file, $args)) {
    return $file;
  }

  return false; 
}

/**
 * 分析字符串 img 标签，并下载到本地
 */
function file_down_image($text, $validate = array(), $rename = 1) {
  global $__file_down_image;
  
  $__file_down_image['validate'] = $validate;
  $__file_down_image['rename'] = $rename;
  
  $text = preg_replace_callback('/(<img.*?src=)(["|\'])(.*?)["|\']/i', '_file_down_image', $text);
  unset($GLOBALS['__file_down_image']);
  
  return $text;
}

function _file_down_image($mat) {
  global $__file_down_image;
	static $files;
  if (!isset($files[$mat[3]])) {
    if (strpos($mat[3], $GLOBALS['base_url']) === false && strpos($mat[3], 'http://') !== false) {
      if ($data = @file_get_contents($mat[3])) {
        
        if (!$__file_down_image['validate']['filename']) {
          $__file_down_image['validate']['filename'] = end(explode('/', $mat[3]));
        } else if (!$__file_down_image['validate']['extension']) {
          $__file_down_image['validate']['extension'] = end(explode('.', $mat[3]));
        }
        
        if ($file = file_save_data($data, $__file_down_image['validate'], $__file_down_image['rename'])) {
          $files[$mat[3]] = $mat[1]. '"' .$GLOBALS['base_path'] .$file->filepath.'"';
        }
      }
    }
    if (!$files[$mat[3]]) {
      $files[$mat[3]] = $mat[0];
    }
  }
  return $files[$mat[3]];
}

/**
 * 数据保存为文件，不做任何验证，文件存在则覆盖
 */
function _file_save_data($data, $filepath) {
  if (!$fp = @fopen($filepath, 'wb')) {
    dd_set_message(t('system', '无法写入文件 %string', array('%string' => $filepath)), 'debug');
    return 0;
  }
  @fwrite($fp, $data);
  @fclose($fp);
  return $filepath;
}

/**
 * 验证图片
 */
function file_validate_image(&$file, $val) {
  if ($data = image_get_info($file->filepath)) {
    $file->filemime = $data['mime_type'];
    $file->extension = $data['extension'];
    foreach ($val as $key => $item) {
      switch ($key) {
        case 'type':
          if (!in_array($data['extension'], $item)) {
            dd_set_message(t('system', '只允许上传 %string 类型的图片', array('%string' => implode(' ', $item))), 'error');
            return false;
          }
        break;
        case 'size':
          $w = explode('x', $item);
          if (($data['width'] > $w[0]) || ($data['height'] > $w[1])) {
            $function = 'image_'. ($val['style'] ? $val['style'] : 'scale');
            if ($function($file->filepath, $file->filepath, $w[0], $w[1], false)) {
              dd_set_message(t('system', '图片长宽超限，已被缩减至 %string', array('%string' => $w[0] .'x'. $w[1])));
            }
          }
        break;
        case 'minsize':
          $w = explode('x', $item);
          if (($data['width'] < $w[0]) || ($data['height'] < $w[1])) {
            dd_set_message(t('system', '图片最小尺寸 %string', 'error', array('%string' => $item)));
            return false;
          }
      }
    }
  } else {
    dd_set_message(t('system', '%string 不是合法的图片文件', array('%string' => $file->filename)), 'error');
    return false;
  }
  return true;
}

/**
 * 验证用户文件空间是否超过限制
 */
function file_validate_usersize($file) {
  if ($file->uid != 1) {
    if ($limit = user_get_file_size($uid)) {
      $sum = db_query('SELECT SUM(filesize) FROM {files} WHERE uid = ?', array($file->uid), array('return' => 'column'));
      if (($file->filesize + $sum) > $limit*1024*1024) {
        return array($limit, $sum/1024/1024);
      }
    }
  }
}

/**
 * 验证文件大小
 */
function file_validate_size(&$file, $val) {
  if (($file->filesize/1024) > $val) {
    return false;
  }
  return $file->filesize;
}

/**
 * 验证文件类型，仅根据文件名判断。
 * @param (object) $file
 *  一个文件对象
 * @param (array) $val
 *  允许的文件类型
 * @return
 *  返回文件 mimetype
 */
function file_validate_mime(&$file, $val = array()) {
  if (!$file->extension && strpos($file->filepath, '.') !== false) {
    $file->extension = end(explode('.', $file->filepath));
  }
  
  if (in_array($file->extension, $val)) {
    $file->filemime = file_get_mimetype($file->extension);
    return $file->filemime;
  }
}

/**
 * 文件上传状态代码解析
 */
function file_validate_error($error) {
  switch ($error) {
    case UPLOAD_ERR_OK:
      return false;
    case UPLOAD_ERR_INI_SIZE:
      return t('system', '上传的文件超过了 php.ini 中 upload_max_filesize 选项限制的值');
    case UPLOAD_ERR_FORM_SIZE:
      return t('system', '上传文件的大小超过了 HTML 表单中 MAX_FILE_SIZE 选项指定的值');
    case UPLOAD_ERR_PARTIAL:
      return t('system', '文件上传不完整');
    case UPLOAD_ERR_NO_TMP_DIR:
      return t('system', '临时文件夹不存在');
    case UPLOAD_ERR_CANT_WRITE:
      return t('system', '文件写入失败');
    case UPLOAD_ERR_NO_FILE: // 4
      return UPLOAD_ERR_NO_FILE;
    default:
      return true;
  }
}

/**
 * 获取用户文件存储目录
 * @param (object) $ac
 *  若无，则取当前用户
 * @return
 *  返回用户存储目录，默认为用户id及年月命名
 */
function file_get_path($ac = NULL) {
  if (!$ac) $ac = $GLOBALS['user'];
  if ($path = var_get('file_user_directory', 'user-[uid]/[date_Y]-[date_m]')) {
    $path = module_invoke('system', 'token_replace', $path);
    $path = module_invoke('user', 'token_replace', $path, $ac);
    if ($path) {
      $path = file_directory_path() . '/' .$path;
    }
  }
  
  if (!$path) {
    $path = file_directory_path() . '/' .$path;
  }
  
	return file_mkdir($path);
}

/**
 * 文件系统根目录
 */
function file_directory_path() {
  return var_get('file_directory_path', $GLOBALS['conf_dir'] . '/files');
}

/**
 * 判断文件夹是否存在，若不存在，则创建
 * @param (string) $path
 *  目录路径
 * @param (int) $mode
 *  新建文件夹权限
 * @param (bool) $recursive
 *  是否递归创建文件夹
 * @return
 *  返回目录路径，若创建失败，则返回 false
 */
function file_mkdir($path, $mode = 0775, $recursive = 1) {
  if (is_dir($path)) {
    return $path;
  } else if (mkdir($path, $mode, $recursive)) {
    return $path;
  } else {
    return false;
  }
}

/**
 * 加载文件
 * @param (int) $fid
 *  文件 Id
 * @param (bool) $invoke
 *  是否触发 hook_file_load()
 */
function file_load($fid, $invoke = true) {
  static $files;

  if (!isset($files[$fid])) {
    if ($files[$fid] = db_query('SELECT f.*, u.name FROM {files} f LEFT JOIN {users} u 
    ON f.uid = u.uid WHERE f.fid = ?', array($fid), array('return' => 'one'))) {
      if ($invoke) module_invoke_all('file_load', $files[$fid]);
    }
  }

  return $files[$fid];
}

/**
 * 删除数据库记录及文件
 * @param (int) $fid
 *  table.files.fid
 * @param (bool) $thumb
 *  是否同时删除缩略图，非图片文件没有缩略图，可传递 false
 * @param (string) $filepath
 *  文件路径，若不传递，将从 files 可查询
 * @return
 *  数据库记录删除成功则返回 true
 */
function file_delete($fid, $thumb = true, $filepath = 0) {
  if (!$filepath) {
    $filepath = db_query('SELECT filepath FROM {files} WHERE fid = ?', array($fid), array('return' => 'column'));
  }
  if (db_exec('DELETE FROM {files} WHERE fid = ?', array($fid))) {
    file_delete_file($filepath, ($thumb ? $fid : 0));
    return true;
  }
}

/**
 * 删除文件及其缩略图
 * @param (string) $filepath
 *  文件路径
 * @param (string) $fid
 *  若传递此值，将尝试删除以 $fid 命名的缩略图文件夹。
 *  通常 $fid 等于 files 表中的 fid，某些时候，数据库中并没有记录，但文件也需要缩略图，fid 则可随意指定
 * @return
 *  文件删除成功返回 true，缩略图文件夹删除操作将静默。
 */ 
function file_delete_file($filepath, $fid = NULL) {
  if ($fid) {
    // 删除缩略图文件夹
    file_delete_dir(dirname($filepath) . '/' . $fid);
  }
  return @unlink($filepath);
}

/**
 * 递归删除目录及其所有文件
 * @param (string) $dir
 *  目录路径
 * @return
 *  静默，并屏蔽错误抛出
 */
function file_delete_dir($dir, $recursive = 1) {
  if (is_dir($dir)) {
    if ($objs = glob($dir . '/*')) {
      foreach ($objs as $obj) {
        if (is_dir($obj)) {
          if ($recursive) file_delete_dir($obj, $recursive);
        } else {
          @unlink($obj);
        }
      }
    }
    @rmdir($dir);
  }
}

/**
 * 删除一组文件
 * @param (array) $files
 *  文件路径
 * @return
 *  删除失败则退出，不再删除后面的文件，并返回 false
 */
function file_delete_array($files = array()) {
  foreach ($files as $file) {
    if (!unlink($file)) {
      return false;
    }
  }
  return true;
}

/**
 * 文件 mime 类型及扩展名
 * @param (string) $key
 *  文件扩展名
 * @param (string) $value
 *  mime 类型
 * @return
 *  传递扩展名，则返回 mime 类型。反之亦然。
 */
function file_get_mimetype($key, $value = 0) {
  $type = array(
    'mimetypes' => array(
      0 => 'application/andrew-inset',
      1 => 'application/atom',
      2 => 'application/atomcat+xml',
      3 => 'application/atomserv+xml',
      4 => 'application/cap',
      5 => 'application/cu-seeme',
      6 => 'application/dsptype',
      7 => 'application/hta',
      8 => 'application/java-archive',
      9 => 'application/java-serialized-object',
      10 => 'application/java-vm',
      11 => 'application/mac-binhex40',
      12 => 'application/mathematica',
      13 => 'application/msaccess',
      14 => 'application/msword',
      15 => 'application/octet-stream',
      16 => 'application/oda',
      17 => 'application/ogg',
      18 => 'application/pdf',
      19 => 'application/pgp-keys',
      20 => 'application/pgp-signature',
      21 => 'application/pics-rules',
      22 => 'application/postscript',
      23 => 'application/rar',
      24 => 'application/rdf+xml',
      25 => 'application/rss+xml',
      26 => 'application/rtf',
      27 => 'application/smil',
      28 => 'application/vnd.cinderella',
      29 => 'application/vnd.google-earth.kml+xml',
      30 => 'application/vnd.google-earth.kmz',
      31 => 'application/vnd.mozilla.xul+xml',
      32 => 'application/vnd.ms-excel',
      33 => 'application/vnd.ms-excel.addin.macroEnabled.12',
      34 => 'application/vnd.ms-excel.sheet.binary.macroEnabled.12',
      35 => 'application/vnd.ms-excel.sheet.macroEnabled.12',
      36 => 'application/vnd.ms-excel.template.macroEnabled.12',
      37 => 'application/vnd.ms-pki.seccat',
      38 => 'application/vnd.ms-pki.stl',
      39 => 'application/vnd.ms-powerpoint',
      40 => 'application/vnd.ms-powerpoint.addin.macroEnabled.12',
      41 => 'application/vnd.ms-powerpoint.presentation.macroEnabled.12',
      42 => 'application/vnd.ms-powerpoint.slideshow.macroEnabled.12',
      43 => 'application/vnd.ms-powerpoint.template.macroEnabled.12',
      44 => 'application/vnd.ms-word.document.macroEnabled.12',
      45 => 'application/vnd.ms-word.template.macroEnabled.12',
      46 => 'application/vnd.ms-xpsdocument',
      47 => 'application/vnd.oasis.opendocument.chart',
      48 => 'application/vnd.oasis.opendocument.database',
      49 => 'application/vnd.oasis.opendocument.formula',
      50 => 'application/vnd.oasis.opendocument.graphics',
      51 => 'application/vnd.oasis.opendocument.graphics-template',
      52 => 'application/vnd.oasis.opendocument.image',
      53 => 'application/vnd.oasis.opendocument.presentation',
      54 => 'application/vnd.oasis.opendocument.presentation-template',
      55 => 'application/vnd.oasis.opendocument.spreadsheet',
      56 => 'application/vnd.oasis.opendocument.spreadsheet-template',
      57 => 'application/vnd.oasis.opendocument.text',
      58 => 'application/vnd.oasis.opendocument.text-master',
      59 => 'application/vnd.oasis.opendocument.text-template',
      60 => 'application/vnd.oasis.opendocument.text-web',
      61 => 'application/vnd.openxmlformats-officedocument.presentationml.presentation',
      62 => 'application/vnd.openxmlformats-officedocument.presentationml.slideshow',
      63 => 'application/vnd.openxmlformats-officedocument.presentationml.template',
      64 => 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
      65 => 'application/vnd.openxmlformats-officedocument.spreadsheetml.template',
      66 => 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
      67 => 'application/vnd.openxmlformats-officedocument.wordprocessingml.template',
      68 => 'application/vnd.rim.cod',
      69 => 'application/vnd.smaf',
      70 => 'application/vnd.stardivision.calc',
      71 => 'application/vnd.stardivision.chart',
      72 => 'application/vnd.stardivision.draw',
      73 => 'application/vnd.stardivision.impress',
      74 => 'application/vnd.stardivision.math',
      75 => 'application/vnd.stardivision.writer',
      76 => 'application/vnd.stardivision.writer-global',
      77 => 'application/vnd.sun.xml.calc',
      78 => 'application/vnd.sun.xml.calc.template',
      79 => 'application/vnd.sun.xml.draw',
      80 => 'application/vnd.sun.xml.draw.template',
      81 => 'application/vnd.sun.xml.impress',
      82 => 'application/vnd.sun.xml.impress.template',
      83 => 'application/vnd.sun.xml.math',
      84 => 'application/vnd.sun.xml.writer',
      85 => 'application/vnd.sun.xml.writer.global',
      86 => 'application/vnd.sun.xml.writer.template',
      87 => 'application/vnd.symbian.install',
      88 => 'application/vnd.visio',
      89 => 'application/vnd.wap.wbxml',
      90 => 'application/vnd.wap.wmlc',
      91 => 'application/vnd.wap.wmlscriptc',
      92 => 'application/wordperfect',
      93 => 'application/wordperfect5.1',
      94 => 'application/x-123',
      95 => 'application/x-7z-compressed',
      96 => 'application/x-abiword',
      97 => 'application/x-apple-diskimage',
      98 => 'application/x-bcpio',
      99 => 'application/x-bittorrent',
      100 => 'application/x-cab',
      101 => 'application/x-cbr',
      102 => 'application/x-cbz',
      103 => 'application/x-cdf',
      104 => 'application/x-cdlink',
      105 => 'application/x-chess-pgn',
      106 => 'application/x-cpio',
      107 => 'application/x-debian-package',
      108 => 'application/x-director',
      109 => 'application/x-dms',
      110 => 'application/x-doom',
      111 => 'application/x-dvi',
      112 => 'application/x-flac',
      113 => 'application/x-font',
      114 => 'application/x-freemind',
      115 => 'application/x-futuresplash',
      116 => 'application/x-gnumeric',
      117 => 'application/x-go-sgf',
      118 => 'application/x-graphing-calculator',
      119 => 'application/x-gtar',
      120 => 'application/x-hdf',
      121 => 'application/x-httpd-eruby',
      122 => 'application/x-httpd-php',
      123 => 'application/x-httpd-php-source',
      124 => 'application/x-httpd-php3',
      125 => 'application/x-httpd-php3-preprocessed',
      126 => 'application/x-httpd-php4',
      127 => 'application/x-ica',
      128 => 'application/x-internet-signup',
      129 => 'application/x-iphone',
      130 => 'application/x-iso9660-image',
      131 => 'application/x-java-jnlp-file',
      132 => 'application/x-javascript',
      133 => 'application/x-jmol',
      134 => 'application/x-kchart',
      135 => 'application/x-killustrator',
      136 => 'application/x-koan',
      137 => 'application/x-kpresenter',
      138 => 'application/x-kspread',
      139 => 'application/x-kword',
      140 => 'application/x-latex',
      141 => 'application/x-lha',
      142 => 'application/x-lyx',
      143 => 'application/x-lzh',
      144 => 'application/x-lzx',
      145 => 'application/x-maker',
      146 => 'application/x-mif',
      147 => 'application/x-ms-wmd',
      148 => 'application/x-ms-wmz',
      149 => 'application/x-msdos-program',
      150 => 'application/x-msi',
      151 => 'application/x-netcdf',
      152 => 'application/x-ns-proxy-autoconfig',
      153 => 'application/x-nwc',
      154 => 'application/x-object',
      155 => 'application/x-oz-application',
      156 => 'application/x-pkcs7-certreqresp',
      157 => 'application/x-pkcs7-crl',
      158 => 'application/x-python-code',
      159 => 'application/x-quicktimeplayer',
      160 => 'application/x-redhat-package-manager',
      161 => 'application/x-shar',
      162 => 'application/x-shockwave-flash',
      163 => 'application/x-stuffit',
      164 => 'application/x-sv4cpio',
      165 => 'application/x-sv4crc',
      166 => 'application/x-tar',
      167 => 'application/x-tcl',
      168 => 'application/x-tex-gf',
      169 => 'application/x-tex-pk',
      170 => 'application/x-texinfo',
      171 => 'application/x-trash',
      172 => 'application/x-troff',
      173 => 'application/x-troff-man',
      174 => 'application/x-troff-me',
      175 => 'application/x-troff-ms',
      176 => 'application/x-ustar',
      177 => 'application/x-wais-source',
      178 => 'application/x-wingz',
      179 => 'application/x-x509-ca-cert',
      180 => 'application/x-xcf',
      181 => 'application/x-xfig',
      182 => 'application/x-xpinstall',
      183 => 'application/xhtml+xml',
      184 => 'application/xml',
      185 => 'application/zip',
      186 => 'audio/basic',
      187 => 'audio/midi',
      346 => 'audio/mp4',
      188 => 'audio/mpeg',
      189 => 'audio/ogg',
      190 => 'audio/prs.sid',
      191 => 'audio/x-aiff',
      192 => 'audio/x-gsm',
      193 => 'audio/x-mpegurl',
      194 => 'audio/x-ms-wax',
      195 => 'audio/x-ms-wma',
      196 => 'audio/x-pn-realaudio',
      197 => 'audio/x-realaudio',
      198 => 'audio/x-scpls',
      199 => 'audio/x-sd2',
      200 => 'audio/x-wav',
      201 => 'chemical/x-alchemy',
      202 => 'chemical/x-cache',
      203 => 'chemical/x-cache-csf',
      204 => 'chemical/x-cactvs-binary',
      205 => 'chemical/x-cdx',
      206 => 'chemical/x-cerius',
      207 => 'chemical/x-chem3d',
      208 => 'chemical/x-chemdraw',
      209 => 'chemical/x-cif',
      210 => 'chemical/x-cmdf',
      211 => 'chemical/x-cml',
      212 => 'chemical/x-compass',
      213 => 'chemical/x-crossfire',
      214 => 'chemical/x-csml',
      215 => 'chemical/x-ctx',
      216 => 'chemical/x-cxf',
      217 => 'chemical/x-embl-dl-nucleotide',
      218 => 'chemical/x-galactic-spc',
      219 => 'chemical/x-gamess-input',
      220 => 'chemical/x-gaussian-checkpoint',
      221 => 'chemical/x-gaussian-cube',
      222 => 'chemical/x-gaussian-input',
      223 => 'chemical/x-gaussian-log',
      224 => 'chemical/x-gcg8-sequence',
      225 => 'chemical/x-genbank',
      226 => 'chemical/x-hin',
      227 => 'chemical/x-isostar',
      228 => 'chemical/x-jcamp-dx',
      229 => 'chemical/x-kinemage',
      230 => 'chemical/x-macmolecule',
      231 => 'chemical/x-macromodel-input',
      232 => 'chemical/x-mdl-molfile',
      233 => 'chemical/x-mdl-rdfile',
      234 => 'chemical/x-mdl-rxnfile',
      235 => 'chemical/x-mdl-sdfile',
      236 => 'chemical/x-mdl-tgf',
      237 => 'chemical/x-mmcif',
      238 => 'chemical/x-mol2',
      239 => 'chemical/x-molconn-Z',
      240 => 'chemical/x-mopac-graph',
      241 => 'chemical/x-mopac-input',
      242 => 'chemical/x-mopac-out',
      243 => 'chemical/x-mopac-vib',
      244 => 'chemical/x-ncbi-asn1-ascii',
      245 => 'chemical/x-ncbi-asn1-binary',
      246 => 'chemical/x-ncbi-asn1-spec',
      247 => 'chemical/x-pdb',
      248 => 'chemical/x-rosdal',
      249 => 'chemical/x-swissprot',
      250 => 'chemical/x-vamas-iso14976',
      251 => 'chemical/x-vmd',
      252 => 'chemical/x-xtel',
      253 => 'chemical/x-xyz',
      254 => 'image/gif',
      255 => 'image/ief',
      256 => 'image/jpeg',
      257 => 'image/pcx',
      258 => 'image/png',
      259 => 'image/svg+xml',
      260 => 'image/tiff',
      261 => 'image/vnd.djvu',
      262 => 'image/vnd.microsoft.icon',
      263 => 'image/vnd.wap.wbmp',
      264 => 'image/x-cmu-raster',
      265 => 'image/x-coreldraw',
      266 => 'image/x-coreldrawpattern',
      267 => 'image/x-coreldrawtemplate',
      268 => 'image/x-corelphotopaint',
      269 => 'image/x-jg',
      270 => 'image/x-jng',
      271 => 'image/x-ms-bmp',
      272 => 'image/x-photoshop',
      273 => 'image/x-portable-anymap',
      274 => 'image/x-portable-bitmap',
      275 => 'image/x-portable-graymap',
      276 => 'image/x-portable-pixmap',
      277 => 'image/x-rgb',
      278 => 'image/x-xbitmap',
      279 => 'image/x-xpixmap',
      280 => 'image/x-xwindowdump',
      281 => 'message/rfc822',
      282 => 'model/iges',
      283 => 'model/mesh',
      284 => 'model/vrml',
      285 => 'text/calendar',
      286 => 'text/css',
      287 => 'text/csv',
      288 => 'text/h323',
      289 => 'text/html',
      290 => 'text/iuls',
      291 => 'text/mathml',
      292 => 'text/plain',
      293 => 'text/richtext',
      294 => 'text/scriptlet',
      295 => 'text/tab-separated-values',
      296 => 'text/texmacs',
      297 => 'text/vnd.sun.j2me.app-descriptor',
      298 => 'text/vnd.wap.wml',
      299 => 'text/vnd.wap.wmlscript',
      300 => 'text/x-bibtex',
      301 => 'text/x-boo',
      302 => 'text/x-c++hdr',
      303 => 'text/x-c++src',
      304 => 'text/x-chdr',
      305 => 'text/x-component',
      306 => 'text/x-csh',
      307 => 'text/x-csrc',
      308 => 'text/x-diff',
      309 => 'text/x-dsrc',
      310 => 'text/x-haskell',
      311 => 'text/x-java',
      312 => 'text/x-literate-haskell',
      313 => 'text/x-moc',
      314 => 'text/x-pascal',
      315 => 'text/x-pcs-gcd',
      316 => 'text/x-perl',
      317 => 'text/x-python',
      318 => 'text/x-setext',
      319 => 'text/x-sh',
      320 => 'text/x-tcl',
      321 => 'text/x-tex',
      322 => 'text/x-vcalendar',
      323 => 'text/x-vcard',
      324 => 'video/3gpp',
      325 => 'video/dl',
      326 => 'video/dv',
      327 => 'video/fli',
      328 => 'video/gl',
      329 => 'video/mp4',
      330 => 'video/mpeg',
      331 => 'video/ogg',
      332 => 'video/quicktime',
      333 => 'video/vnd.mpegurl',
      347 => 'video/x-flv',
      334 => 'video/x-la-asf',
      335 => 'video/x-mng',
      336 => 'video/x-ms-asf',
      337 => 'video/x-ms-wm',
      338 => 'video/x-ms-wmv',
      339 => 'video/x-ms-wmx',
      340 => 'video/x-ms-wvx',
      341 => 'video/x-msvideo',
      342 => 'video/x-sgi-movie',
      343 => 'x-conference/x-cooltalk',
      344 => 'x-epoc/x-sisx-app',
      345 => 'x-world/x-vrml',
      348 => 'application/iac'
    ),
    'extensions' => array(
      'ez' => 0,
      'atom' => 1,
      'atomcat' => 2,
      'atomsrv' => 3,
      'cap' => 4,
      'pcap' => 4,
      'cu' => 5,
      'tsp' => 6,
      'hta' => 7,
      'jar' => 8,
      'ser' => 9,
      'class' => 10,
      'hqx' => 11,
      'nb' => 12,
      'mdb' => 13,
      'dot' => 14,
      'doc' => 14,
      'bin' => 15,
      'oda' => 16,
      'ogg' => 17,
      'ogx' => 17,
      'pdf' => 18,
      'key' => 19,
      'pgp' => 20,
      'prf' => 21,
      'eps' => 22,
      'ai' => 22,
      'ps' => 22,
      'rar' => 23,
      'rdf' => 24,
      'rss' => 25,
      'rtf' => 26,
      'smi' => 27,
      'smil' => 27,
      'cdy' => 28,
      'kml' => 29,
      'kmz' => 30,
      'xul' => 31,
      'xlb' => 32,
      'xlt' => 32,
      'xls' => 32,
      'xlam' => 33,
      'xlsb' => 34,
      'xlsm' => 35,
      'xltm' => 36,
      'cat' => 37,
      'stl' => 38,
      'pps' => 39,
      'ppt' => 39,
      'ppam' => 40,
      'pptm' => 41,
      'ppsm' => 42,
      'potm' => 43,
      'docm' => 44,
      'dotm' => 45,
      'xps' => 46,
      'odc' => 47,
      'odb' => 48,
      'odf' => 49,
      'odg' => 50,
      'otg' => 51,
      'odi' => 52,
      'odp' => 53,
      'otp' => 54,
      'ods' => 55,
      'ots' => 56,
      'odt' => 57,
      'odm' => 58,
      'ott' => 59,
      'oth' => 60,
      'pptx' => 61,
      'ppsx' => 62,
      'potx' => 63,
      'xlsx' => 64,
      'xltx' => 65,
      'docx' => 66,
      'dotx' => 67,
      'cod' => 68,
      'mmf' => 69,
      'sdc' => 70,
      'sds' => 71,
      'sda' => 72,
      'sdd' => 73,
      'sdw' => 75,
      'sgl' => 76,
      'sxc' => 77,
      'stc' => 78,
      'sxd' => 79,
      'std' => 80,
      'sxi' => 81,
      'sti' => 82,
      'sxm' => 83,
      'sxw' => 84,
      'sxg' => 85,
      'stw' => 86,
      'sis' => 87,
      'vsd' => 88,
      'wbxml' => 89,
      'wmlc' => 90,
      'wmlsc' => 91,
      'wpd' => 92,
      'wp5' => 93,
      'wk' => 94,
      '7z' => 95,
      'abw' => 96,
      'dmg' => 97,
      'bcpio' => 98,
      'torrent' => 99,
      'cab' => 100,
      'cbr' => 101,
      'cbz' => 102,
      'cdf' => 103,
      'vcd' => 104,
      'pgn' => 105,
      'cpio' => 106,
      'udeb' => 107,
      'deb' => 107,
      'dir' => 108,
      'dxr' => 108,
      'dcr' => 108,
      'dms' => 109,
      'wad' => 110,
      'dvi' => 111,
      'flac' => 112,
      'pfa' => 113,
      'pfb' => 113,
      'pcf' => 113,
      'gsf' => 113,
      'pcf.z' => 113,
      'mm' => 114,
      'spl' => 115,
      'gnumeric' => 116,
      'sgf' => 117,
      'gcf' => 118,
      'taz' => 119,
      'gtar' => 119,
      'tgz' => 119,
      'hdf' => 120,
      'rhtml' => 121,
      'phtml' => 122,
      'pht' => 122,
      'php' => 122,
      'phps' => 123,
      'php3' => 124,
      'php3p' => 125,
      'php4' => 126,
      'ica' => 127,
      'ins' => 128,
      'isp' => 128,
      'iii' => 129,
      'iso' => 130,
      'jnlp' => 131,
      'js' => 132,
      'jmz' => 133,
      'chrt' => 134,
      'kil' => 135,
      'skp' => 136,
      'skd' => 136,
      'skm' => 136,
      'skt' => 136,
      'kpr' => 137,
      'kpt' => 137,
      'ksp' => 138,
      'kwd' => 139,
      'kwt' => 139,
      'latex' => 140,
      'lha' => 141,
      'lyx' => 142,
      'lzh' => 143,
      'lzx' => 144,
      'maker' => 145,
      'frm' => 145,
      'frame' => 145,
      'fm' => 145,
      'book' => 145,
      'fb' => 145,
      'fbdoc' => 145,
      'mif' => 146,
      'wmd' => 147,
      'wmz' => 148,
      'dll' => 149,
      'bat' => 149,
      'exe' => 149,
      'com' => 149,
      'msi' => 150,
      'nc' => 151,
      'pac' => 152,
      'nwc' => 153,
      'o' => 154,
      'oza' => 155,
      'p7r' => 156,
      'crl' => 157,
      'pyo' => 158,
      'pyc' => 158,
      'qtl' => 159,
      'rpm' => 160,
      'shar' => 161,
      'swf' => 162,
      'swfl' => 162,
      'sitx' => 163,
      'sit' => 163,
      'sv4cpio' => 164,
      'sv4crc' => 165,
      'tar' => 166,
      'gf' => 168,
      'pk' => 169,
      'texi' => 170,
      'texinfo' => 170,
      'sik' => 171,
      '~' => 171,
      'bak' => 171,
      '%' => 171,
      'old' => 171,
      't' => 172,
      'roff' => 172,
      'tr' => 172,
      'man' => 173,
      'me' => 174,
      'ms' => 175,
      'ustar' => 176,
      'src' => 177,
      'wz' => 178,
      'crt' => 179,
      'xcf' => 180,
      'fig' => 181,
      'xpi' => 182,
      'xht' => 183,
      'xhtml' => 183,
      'xml' => 184,
      'xsl' => 184,
      'zip' => 185,
      'au' => 186,
      'snd' => 186,
      'mid' => 187,
      'midi' => 187,
      'kar' => 187,
      'mpega' => 188,
      'mpga' => 188,
      'm4a' => 188,
      'mp3' => 188,
      'mp2' => 188,
      'oga' => 189,
      'spx' => 189,
      'sid' => 190,
      'aif' => 191,
      'aiff' => 191,
      'aifc' => 191,
      'gsm' => 192,
      'm3u' => 193,
      'wax' => 194,
      'wma' => 195,
      'rm' => 196,
      'ram' => 196,
      'ra' => 197,
      'pls' => 198,
      'sd2' => 199,
      'wav' => 200,
      'alc' => 201,
      'cac' => 202,
      'cache' => 202,
      'csf' => 203,
      'cascii' => 204,
      'cbin' => 204,
      'ctab' => 204,
      'cdx' => 205,
      'cer' => 206,
      'c3d' => 207,
      'chm' => 208,
      'cif' => 209,
      'cmdf' => 210,
      'cml' => 211,
      'cpa' => 212,
      'bsd' => 213,
      'csml' => 214,
      'csm' => 214,
      'ctx' => 215,
      'cxf' => 216,
      'cef' => 216,
      'emb' => 217,
      'embl' => 217,
      'spc' => 218,
      'gam' => 219,
      'inp' => 219,
      'gamin' => 219,
      'fchk' => 220,
      'fch' => 220,
      'cub' => 221,
      'gau' => 222,
      'gjf' => 222,
      'gjc' => 222,
      'gal' => 223,
      'gcg' => 224,
      'gen' => 225,
      'hin' => 226,
      'istr' => 227,
      'ist' => 227,
      'dx' => 228,
      'jdx' => 228,
      'kin' => 229,
      'mcm' => 230,
      'mmd' => 231,
      'mmod' => 231,
      'mol' => 232,
      'rd' => 233,
      'rxn' => 234,
      'sdf' => 235,
      'sd' => 235,
      'tgf' => 236,
      'mcif' => 237,
      'mol2' => 238,
      'b' => 239,
      'gpt' => 240,
      'mopcrt' => 241,
      'zmt' => 241,
      'mpc' => 241,
      'dat' => 241,
      'mop' => 241,
      'moo' => 242,
      'mvb' => 243,
      'prt' => 244,
      'aso' => 245,
      'val' => 245,
      'asn' => 246,
      'ent' => 247,
      'pdb' => 247,
      'ros' => 248,
      'sw' => 249,
      'vms' => 250,
      'vmd' => 251,
      'xtel' => 252,
      'xyz' => 253,
      'gif' => 254,
      'ief' => 255,
      'jpeg' => 256,
      'jpe' => 256,
      'jpg' => 256,
      'pcx' => 257,
      'png' => 258,
      'svgz' => 259,
      'svg' => 259,
      'tif' => 260,
      'tiff' => 260,
      'djvu' => 261,
      'djv' => 261,
      'ico' => 262,
      'wbmp' => 263,
      'ras' => 264,
      'cdr' => 265,
      'pat' => 266,
      'cdt' => 267,
      'cpt' => 268,
      'art' => 269,
      'jng' => 270,
      'bmp' => 271,
      'psd' => 272,
      'pnm' => 273,
      'pbm' => 274,
      'pgm' => 275,
      'ppm' => 276,
      'rgb' => 277,
      'xbm' => 278,
      'xpm' => 279,
      'xwd' => 280,
      'eml' => 281,
      'igs' => 282,
      'iges' => 282,
      'silo' => 283,
      'msh' => 283,
      'mesh' => 283,
      'icz' => 285,
      'ics' => 285,
      'css' => 286,
      'csv' => 287,
      '323' => 288,
      'html' => 289,
      'htm' => 289,
      'shtml' => 289,
      'uls' => 290,
      'mml' => 291,
      'txt' => 292,
      'conf' => 292,
      'pot' => 292,
      'text' => 292,
      'asc' => 292,
      'rtx' => 293,
      'wsc' => 294,
      'sct' => 294,
      'tsv' => 295,
      'ts' => 296,
      'tm' => 296,
      'jad' => 297,
      'wml' => 298,
      'wmls' => 299,
      'bib' => 300,
      'boo' => 301,
      'hpp' => 302,
      'hh' => 302,
      'h++' => 302,
      'hxx' => 302,
      'cxx' => 303,
      'cc' => 303,
      'cpp' => 303,
      'c++' => 303,
      'h' => 304,
      'htc' => 305,
      'csh' => 306,
      'c' => 307,
      'patch' => 308,
      'diff' => 308,
      'd' => 309,
      'hs' => 310,
      'java' => 311,
      'lhs' => 312,
      'moc' => 313,
      'pas' => 314,
      'p' => 314,
      'gcd' => 315,
      'pm' => 316,
      'pl' => 316,
      'py' => 317,
      'etx' => 318,
      'sh' => 319,
      'tk' => 320,
      'tcl' => 320,
      'cls' => 321,
      'ltx' => 321,
      'sty' => 321,
      'tex' => 321,
      'vcs' => 322,
      'vcf' => 323,
      '3gp' => 324,
      'dl' => 325,
      'dif' => 326,
      'dv' => 326,
      'fli' => 327,
      'gl' => 328,
      'mp4' => 329,
      'f4v' => 329,
      'f4p' => 329,
      'mpe' => 330,
      'mpeg' => 330,
      'mpg' => 330,
      'ogv' => 331,
      'qt' => 332,
      'mov' => 332,
      'mxu' => 333,
      'lsf' => 334,
      'lsx' => 334,
      'mng' => 335,
      'asx' => 336,
      'asf' => 336,
      'wm' => 337,
      'wmv' => 338,
      'wmx' => 339,
      'wvx' => 340,
      'avi' => 341,
      'movie' => 342,
      'ice' => 343,
      'sisx' => 344,
      'wrl' => 345,
      'vrm' => 345,
      'vrml' => 345,
      'f4a' => 346,
      'f4b' => 346,
      'flv' => 347,
      'iac' => 348,
    ),
  );
  
  if ($key) {
    $key = strtolower($key);
    if (isset($type['extensions'][$key])) {
      return $type['mimetypes'][$type['extensions'][$key]];
    }
  } else if ($value) {
    $value = strtolower($value);
    $key = array_search($value, $type['mimetypes']);
    if ($key !== false) {
      return array_search($key, $type['extensions']);
    }
  }
}
