<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <title><?php echo $head_title; ?></title>
  <?php echo $heads; ?>
  <?php echo $styles; ?>
  <?php echo $scripts; ?>
</head>
<body<?php echo $sidebars_class;?>>
	<div id="wrapper"><div id="page_wrapper_<?php echo $args_id;?>">
		<div id="header"><?php echo $top; ?></div>
		<div id="main"> 
		 <?php if ($sidebars_class) : ?><div id="content"><?php endif; ?>
				<div id="center">
					<div class="column">
            <?php echo $breadcrumb; ?>
            <?php echo $tabs; ?>
            <?php echo $sub_tabs; ?>
            <?php echo $messages; ?>
            <?php echo $help; ?>
            <?php echo $content; ?>
					</div>
				</div>
				<?php if ($right) : ?>
					<div id="sidebar-right" class="sidebar">
						<div class="column">
							<?php echo $right; ?>
						</div>
					</div>
				<?php endif; ?>
			<?php if ($sidebars_class) : ?></div><?php endif; ?>
			
			<?php if ($left) : ?>
				<div id="sidebar-left" class="sidebar">
					<div class="column">
						<?php echo $left; ?>
					</div>
				</div>
			<?php endif; ?>
		</div>
		<div id="footer"><?php echo $bottom; ?></div>
	</div></div>
	<?php echo $closure; ?>
</body>
</html>