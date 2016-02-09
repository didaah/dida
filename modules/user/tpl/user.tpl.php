<?php
// $Id$
// print_r($account);
?>

<h2><?php echo $account->name; ?></h2>

<?php if ($account->content) : ?>
  <?php echo $account->content; ?>
<?php endif;?>