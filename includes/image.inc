<?php
// $Id$

/**
 * 获取缩略图，若不存在，则生成，返回缩略图路径，若生成不成功，返回原图。原图不存在，返回错误图片
 * @param (int or string) $fid
 *  图片 id，一般指定 files 表 fid，也可自定义，用于创建缩略图存放目录
 * @param (string) $filepath
 *  图片路径，相对路径
 * @param (string) $size
 *  缩略图大小，如：100x100
 * @param (string) $style
 *  缩略图风格，scale、scale_resize、scale_and_crop、crop、resize 可选
 */
function image_get_thumb($fid, $filepath, $size, $style = 'scale') {
  static $files;
  if (!isset($files[$fid]) || !isset($files[$fid][$size][$style])) {
    if (empty($files[$fid])) $files[$fid] = array($size => array());
    if (is_file($filepath)) {
      $files[$fid][$size][$style] = $filepath;
      $filename = basename($filepath);
      $thumb_filepath = file_mkdir(str_replace($filename, $fid, $filepath)) . '/';
      $thumb_filepath .= $size . '_' . $style;
      
      if (strpos($filename, '.') !== false) {
        $_ext = explode('.', $filename);
        $thumb_filepath .= '.' . end($_ext);
      }

      if (is_file($thumb_filepath)) {
        $files[$fid][$size][$style] = $thumb_filepath;
      } else {
        $t = explode('x', $size);
        $function = 'image_' . $style;
        if (function_exists($function)) {
          $true = $function($filepath, $thumb_filepath, $t[0], $t[1]);
        } else {
          $true = image_scale_resize($filepath, $thumb_filepath, $t[0], $t[1]);
        }
        if ($true) $files[$fid][$size][$style] = $thumb_filepath;
      }
    } else {
      $files[$fid][$size][$style] = 'custom.php?module=system&op=image_error&size=' . $size;
    }
  }

  return $files[$fid][$size][$style];
}

/**
 * 获取图片信息 
 * @param (string) $file 
 *  图片路径，php 须有读写权限
 * @return (array)
 */
function image_get_info($file) {
  $details = false;
  $data = @getimagesize($file);
  $file_size = @filesize($file);

  if (isset($data) && is_array($data)) {
    $extensions = array('1' => 'gif', '2' => 'jpg', '3' => 'png');
    $details = array(
      'width' => $data[0],
      'height' => $data[1],
      'file_size' => $file_size,
      'mime_type' => $data['mime'],
   );
    if (array_key_exists($data[2], $extensions)) {
      $details['extension'] = $extensions[$data[2]];
    }
  }
  return $details;
}

/**
 * 剪裁图片，在保证尺寸的前提下，最大限度保持比例
 * @param string $source 
 *  待剪裁的图片路径
 * @param string $destination 
 *  剪裁后图片的保存位置
 * @param int $width 
 *  剪裁后的图片宽度
 * @param int $height 
 *  剪裁后的图片高度
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_scale_resize($source, $destination, $width, $height, $mark = NULL) {
  $info = image_get_info($source);
  $aspect = $info['height'] / $info['width'];
  if ($aspect != $height / $width) {
    if ($info['width'] >= $width || $info['height'] >= $height) {
      if (image_scale($source, $destination, $width, $height, $mark)) {
        $source = $destination;
        $info = image_get_info($destination);
      }
    }
    $dst = image_gd_open($source, $info['extension']);
    if (!$dst) {
      return false;
    }
    $im = imagecreatetruecolor($width, $height);
    $color = imagecolorallocate($im, 255, 255, 255);
    imagefill($im, 0, 0, $color);
    
    $dst_x = ($width - $info['width']) / 2;
    $dst_y = ($height - $info['height']) / 2;
    
    imagecopy($im, $dst, $dst_x, $dst_y, 0, 0, $info['width'], $info['height']);
    $result = image_gd_close($im, $source, $info['extension']);
    
    imagedestroy($im);
    imagedestroy($dst);
    return $result;
  } else {
    return image_gd_resize($source, $destination, $width, $height, $mark);
  }
}

/**
 * 剪裁图片，保证图片尺寸，也保持原图比例，若比例不一致，则进行二次剪裁
 * @param string $source 
 *  待剪裁的图片路径
 * @param string $destination 
 *  剪裁后图片的保存位置
 * @param int $width 
 *  剪裁后的图片宽度
 * @param int $height 
 *  剪裁后的图片高度
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_scale_and_crop($source, $destination, $width, $height, $mark = NULL) {
  $info = image_get_info($source);

  $scale = max($width / $info['width'], $height / $info['height']);
  $x = round(($info['width'] * $scale - $width) / 2);
  $y = round(($info['height'] * $scale - $height) / 2);

  if (image_gd_resize($source, $destination, $info['width'] * $scale, $info['height'] * $scale, $mark)) {
    return image_gd_crop($destination, $destination, $x, $y, $width, $height, $mark);
  }
  return false;
}

/**
 * 剪裁图片，保持图片比例
 * @param string $source 
 *  待剪裁的图片路径
 * @param string $destination 
 *  剪裁后图片的保存位置
 * @param int $width 
 *  剪裁后的图片宽度
 * @param int $height 
 *  剪裁后的图片高度
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_scale($source, $destination, $width, $height, $mark = NULL) {
  $info = image_get_info($source);
  
  if ($width >= $info['width'] && $height >= $info['height']) {
    return false;
  }
  $aspect = $info['height'] / $info['width'];
  if ($aspect < $height / $width) {
    $width = (int)min($width, $info['width']);
    $height = (int)round($width * $aspect);
  } else {
    $height = (int)min($height, $info['height']);
    $width = (int)round($height / $aspect);
  }
  
  return image_gd_resize($source, $destination, $width, $height, $mark);
}

/**
 * 剪裁图片，保证尺寸
 * @param string $source 
 *  待剪裁的图片路径
 * @param string $destination 
 *  剪裁后图片的保存位置
 * @param int $width 
 *  剪裁后的图片宽度
 * @param int $height 
 *  剪裁后的图片高度
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_resize($source, $destination, $width, $height, $mark = NULL) {
  return image_gd_resize($source, $destination, $width, $height, $mark);
}

/**
 * 旋转图片
 * @param string $source 
 *  待旋转的图片路径
 * @param string $destination 
 *  旋转后图片的保存位置
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_rotate($source, $destination, $degrees, $background = 0x000000, $mark = NULL) {
  return image_gd_rotate($source, $destination, $degrees, $background, $mark);
}

/**
 * 按给定坐标剪裁图片
 * @param string $source 
 *  待剪裁的图片路径
 * @param string $destination 
 *  剪裁后图片的保存位置
 * @param int $x
 *  剪裁 x 坐标
 * @param int $y
 *  剪裁 y 坐标
 * @param int $width 
 *  剪裁后的图片宽度
 * @param int $height 
 *  剪裁后的图片高度
 * @param bool $mark 
 *  是否添加水印，若不设置，则使用全局设置
 * @access public
 * @return bool
 */
function image_crop($source, $destination, $x, $y, $width, $height, $mark = NULL) {
  return image_gd_crop($source, $destination, $x, $y, $width, $height, $mark);
}

/**
 * 转换图片格式
 * @param (string) $source
 *  原图片
 * @param (string) $ext
 *  原扩展名
 * @param (string) $newext
 *  新扩展名
 * @param (string) $newsource
 *  新的位置
 */
function image_to_type_data($source, $ext, $newext, $newsource) {
  if ($ext != $newext) {
    $function = 'imagecreatefrom' . $ext;
    $functionnew = 'image' . ($newext == 'jpg' ? 'jpeg' : $newext);
    if (function_exists($function) && function_exists($functionnew)) {
      if ($im = $function($source)) {
        if ($functionnew($im, $newsource)) {
          imagedestroy($im);
          return true;
        }
      }
    }
  } else {
    return file_save_move($source, array('filepath' => $newsource, 'no_insert' => 1), 0);
  }
}

/**
 * GD 库接口
 */

/**
 * 裁剪，固定大小
 */
function image_gd_resize($source, $destination, $width, $height, $mark = NULL) {
  if (!file_exists($source)) {
    return false;
  }
  $info = image_get_info($source);
  if (!$info) {
    return false;
  }
  $im = image_gd_open($source, $info['extension']);
  if (!$im) {
    return false;
  }
  $res = imagecreatetruecolor($width, $height);
  if ($info['extension'] == 'png') {
    $transparency = imagecolorallocatealpha($res, 255, 255, 255, 127);
    imagealphablending($res, false);
    imagefilledrectangle($res, 0, 0, $width, $height, $transparency);
    imagealphablending($res, TRUE);
    imagesavealpha($res, TRUE);
  } elseif ($info['extension'] == 'gif') {
    $transparency_index = imagecolortransparent($im);
    if ($transparency_index >= 0) {
      $transparent_color = imagecolorsforindex($im, $transparency_index);
      $transparency_index = imagecolorallocate($res, $transparent_color['red'], $transparent_color['green'], $transparent_color['blue']);
      imagefill($res, 0, 0, $transparency_index);
      imagecolortransparent($res, $transparency_index);
      $number_colors = imagecolorstotal($im);
      imagetruecolortopalette($res, TRUE, $number_colors);
    }
  }
  
  imagecopyresampled($res, $im, 0, 0, 0, 0, $width, $height, $info['width'], $info['height']);

  $result = image_gd_close($res, $destination, $info['extension']);

  imagedestroy($res);
  imagedestroy($im);

  // 添加水印
  if (!empty($mark) || ($mark !== false && var_get_key('file_image_watermark', 'default', 0))) {
    image_gd_mark($destination, array());
  }

  return $result;
}

/**
 * 图片水印
 * @param (string) $filepath
 * 	文件路径，必须是 php 有权限读写操作的文件
 * @param (array) $args
 *  水印设置，所有参数皆可为空，默认使用全局配置 var_get('file_image_watermark')
 *  array(
 *    'garble' => true|false, // 是否随机添加混淆水印
 *    'min_width' => 150, // 最小宽度，小于此宽度的图片不添加水印
 *    'min_height' => 150, // 最小高度，小于此高度的图片不添加水印
 *    'file' => 'misc/test.png', // 主水印图片，必须是 png 格式且 php 有权限读写的文件
 *    'garble_file' => 'misc/test.pnc', // 混淆水印图片，同上
 *    'width' => 100, // 主水印图片宽度，若自定义水印图片，则应该提供此值
 *    'height' => 100, // 主水印图片高度，同上
 *    'garble_width' => 50, // 同上
 *    'garble_height' => 50, // 同上
 *  )
 * @param (resource) $im
 *  GD 图片数据，若 $filepath 已打开，可直接传递 GD 数据
 * @return (bool|resource)
 */
function image_gd_mark($filepath, array $args = array(), $im = NULL) {
  $opt = array_merge(var_get('file_image_watermark', array()), $args);

  if (empty($opt['file'])) {
    dd_set_message(t('system', '水印图片不存在'), 'error');
    return false;
  }

  $info = image_get_info($filepath);
  
  if ($info['width'] > $opt['min_width'] && $info['height'] > $opt['min_height']) {
    if (empty($im)) {
      $_im = image_gd_open($filepath, $info['extension']);
    } else {
      $_im = $im;
    }

    if (!$_im) return false;
    
    if ($opt['garble'] && is_file($opt['garble_file'])) {
      $watermark = imagecreatefrompng($opt['garble_file']);
      
      $i = $_width = 0;
      $_width_row = rand(80, 150);
      
      while ($_width < $info['width']) {
        if ($i > 30) break;
        $_height = rand(60, 100);
        $_height_count = floor($info['height']/$_height);
        while ($_height_count--) {
          imagecopy($_im, $watermark, $_width, $_height*$_height_count, 0, 0, 
            $opt['garble_width'], $opt['garble_height']);
        }
        $_width += $_width_row;
        ++$i;
      }
      
      imagedestroy($watermark);
    }
    
    $watermark = imagecreatefrompng($opt['file']);
    
    imagecopy($_im, $watermark, $info['width']-$opt['width']-5, 
      $info['height']-$opt['height']-5, 0, 0, $opt['width'], $opt['height']);

    imagedestroy($watermark);

    if ($im) return $_im;

    $result = image_gd_close($_im, $filepath, $info['extension']);

    imagedestroy($_im);
    
    return $result;
  }
}

/**
 * 旋转图像
 */
function image_gd_rotate($source, $destination, $degrees, $background = 0xFFFFFF, $mark = NULL) {
  if (!function_exists('imageRotate')) {
    return false;
  }

  $info = image_get_info($source);
  if (!$info) {
    return false;
  }

  $im = image_gd_open($source, $info['extension']);
  if (!$im) {
    return false;
  }

  $res = imageRotate($im, $degrees, $background);
  $result = image_gd_close($res, $destination, $info['extension']);

  return $result;
}

/**
 * 裁剪图片
 */
function image_gd_crop($source, $destination, $x, $y, $width, $height, $mark = NULL) {
  $info = image_get_info($source);
  if (!$info) {
    return false;
  }

  $im = image_gd_open($source, $info['extension']);
  $res = imageCreateTrueColor($width, $height);
  imageCopy($res, $im, 0, 0, $x, $y, $width, $height);

  $result = image_gd_close($res, $destination, $info['extension']);

  imageDestroy($res);
  imageDestroy($im);

  return $result;
}

function image_gd_open($file, $extension) {
  $extension = str_replace('jpg', 'jpeg', $extension);
  $open_func = 'imageCreateFrom'. $extension;
  if (!function_exists($open_func)) {
    return false;
  }
  return $open_func($file);
}

function image_gd_close($res, $destination, $extension) {
  $extension = str_replace('jpg', 'jpeg', $extension);
  $close_func = 'image'. $extension;
  if (!function_exists($close_func)) {
    return false;
  }
  if ($extension == 'jpeg') {
    return $close_func($res, $destination, var_get('file_image_quality', 75));
  } else {
    return $close_func($res, $destination);
  }
}
