<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <title><?php echo $head_title; ?></title>
  <?php echo $heads; ?>
  <?php echo $styles; ?>
  <?php echo $scripts; ?>
</head>
  <body class="<?php echo $user_login_status;?>">
	<div id="wrapper" class="container"><div class="<?php echo $args_id;?>">
		<div id="header">
      <?php echo $lang_links; ?>
      <div class="menu"><?php echo $menu; ?></div>
      <a href="<?php echo $base_path?>" title="回到 <?php echo $site_global->name; ?> 首页" class="logo">
        <img src="<?php echo $site_global->logo; ?>" alt="<?php echo $site_global->name?>" />
      </a>
    </div>
    
    <div id="main"> 
      <?php if ($left) : ?>
				<div id="sidebar-left" class="sidebar span-5">
          <?php echo $left; ?>
        </div>
			<?php endif; ?>

      <?php
        if ($left && $right) {
          $content_class = 'span-14';
        } else if ($left) {
          $content_class = 'span-19 last';
        } else if ($right) {
          $content_class = 'span-19';
        } else {
          $content_class = '';
        }
      ?>
      
      <div id="content" class="<?php echo $content_class;?>">
        <?php echo $breadcrumb; ?>
        <?php echo $tabs; ?>
        <?php echo $sub_tabs; ?>
        <?php echo $messages; ?>
        <?php echo $help; ?>
        <?php echo $content; ?>
      </div>
      
      <?php if ($right) : ?>
        <div id="sidebar-right" class="sidebar span-5 last">
          <?php echo $right; ?>
        </div>
      <?php endif; ?>
			
		</div>
		<div id="footer"><?php echo $site_global->footer; ?><?php echo $footer; ?><?php echo $debug; ?></div>
	</div></div>
	<?php echo $closure; ?>
</body>
</html>
