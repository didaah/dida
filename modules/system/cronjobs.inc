<?php
// $Id$

/**
 * @file 自定义计划任务
 *  通过 hook_cronjobs() 定义的计划任务，可以灵活设置执行时间
 *  这种方式通常用于数据采集、挖掘等操作，每个任务必须对应模块 scripts 目录中的一个 php 文件
 *  cron.inc 在执行时，将调用 php 运行该文件，单独生成一个进程，便于管理，且减少相互影响
 */

/**
 * @Implement of hook_cronjobs()
 */
function system_cronjobs() {

  return false; // 请参见以下代码

  $cronjobs = array();

  // 定义一个任务，任务机器名为 test
  // 可通过 timestamp['test'] 获取本任务上一次执行时间，可能为 0
  $cronjobs['test'] = array(
    'name' => t('system', '测试任务'), // 显示名，必需
    'script' => 'test.inc', // 必需，对应的脚本，该脚本必须位于模块的 scripts 目录，如：system/scripts，将调用 php 执行该脚本
    'param' => array('date' => 20120102, 'id' => 'name'), // 可选参数，数据类型：array。将做为参数传递给 scripts
    'minHour' => 13, // 可选参数，设置最小时间，当前小时必须大于该数字，才启动任务
    'year' => '*', // 年，每一年
    'month' => '*', // 月，每一月
    'day' => '*', // 日，每一天
    //'day' => '*/5', // 每 5 天
    //'day' => '12', // 每月 12 号
    'minute' => '5', // 分，0-59
  );
  /**
   * 以上 test 任务，基本等同于 5 * * * * php {$path}/test.inc --date 20120102 --id 100
   * 以这种 Dida 内部方式实现，比直接在系统 crontab 中添加任务的好处在于：
   *  1、统一管理，Dida 可将所有模块的任务统一调度，根据系统负载，其它任务的执行情况来合理安排；
   *  2、便捷，管理员只需要添加一条 php cron.inc 即可，省去繁琐的操作
   * 因此，建议所有模块开发者，有较为复杂繁琐的计划任务，均使用此种方式实现 
   *
   * 注意：因为这是一个测试任务，所以并没有在 system 模块中创建 scripts 目录和 test.inc 文件
   */

  return $cronjobs;
}
