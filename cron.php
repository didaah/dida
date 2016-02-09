<?php
// $Id$

/**
 * 在 Dida 中，实现计划任务的方式分为三种
 *  1、保存一个任务到 cron 表；
 *  2、实现 hook_cron()，定义计划任务
 *  3、在模块下创建文件 cronjobs.inc 定义更复杂的任务
 *
 * 1、保存一个任务到 cron 表
 *  cron 为任务表，每条记录为一个任务。每个模块都可以向该表写入任务
 *  也可以调用 dd_save_cron() 保存任务，具体格式见 bootstrap.inc dd_save_cron()
 *  每次执行计划任务时，优化读取 cron 表中所有未完成的任务，予以执行
 *  建议将耗费时间较短、非重复性的任务以写入 cron 表的方式执行
 *  执行时间：每次执行 cron.php cron.inc 时均会调用
 * 2、实现 hook_cron()，定义计划任务
 *  假如需要耗费较长时间的复杂繁琐任务，如备份、数据采集等
 *  或者任务是一项长期且重复性的工作，如清空临时表，检查过期缩略图等
 *  建议实现 hook_cron() 接口。在 hook_cron() 中自行处理更为合适
 *  执行时间：遵循管理员所设置的时间限制
 * 3、在模块下创建文件 cronjobs.inc 定义更复杂的任务
 *  假如需要更细腻的任务控制，比如某个行为，只需要每天执行一次，或者每天早上和夜里各执行一次
 *  通常的做法是在操作系统的 cronjobs 中添加一条任务，但假如有许多类似任务，将不便于管理
 *  故而可使用本方法，在模块目录下创建  cronjobs.inc 文件，定义函数 hook_cronjobs() 
 *  详细设置方法参见 system.module 模块中的 cronjobs.inc 示例文件
 *  注意：必须在系统中 cron 中添加一条任务 php $path/scripts/cron.inc --host www.didaah.org 才能生效
 *  每次运行 scripts/cron.inc 时，将遍历所有模块下的 cronjobs.inc 文件，安排完成指定的操作
 *  该方法适合有大量计划任务的模块，比如数据采集、每日数据统计等
 *  优点在于可统一用 cron.inc 安排任务的执行，可根据系统负载来合理调度，让计划任务更加有序
 *  缺点在于可能并不即时，例如在 hook_cronjobs() 中设置 13 点执行 A 任务
 *  但可能当时其它执行中的任务较多，系统负载较高，cron.inc 将会推迟 A 任务的执行
 *  执行时间：自定义，不遵循管理员所设置的时间限制
 *
 * 模块开发者可根据自身需求，选择任意一种方式来实现计划任务。
 *
 * @param int $timestamp
 *  hook_cron() 将传递一个时间戳，即上一次 cron 执行的时间
 *  hook_cron($timestamp) {
 *    db_query('DELETE FROM {temp} ....'); // 定期清除临时表
 *  }
 */

define('DIDA_ROOT', getcwd());
require_once DIDA_ROOT . '/includes/bootstrap.inc';
bootstrap('full');

/**
 * 获得 cron 的执行密码，默认为所有人可执行
 * 若设置了密码，则必须传递 URL 参数：pass
 * 密码生成方式：自定义字符串的 md5 值
 * cron 并不会泄露安全信息，故而这仅是一个很简单的验证
 */

if ($pass = var_get('cron_pass', false)) {
  if (!$_GET['pass'] || $_GET['pass'] != md5($pass)) {
    echo 'byebye';
    exit;
  }
}

// 获得上一次执行时间
$timestamp = var_get('cron_last_time', 0);

/**
 * 获得最小执行时间，默认为 3600 秒，防止频繁执行
 * $user->uid == 1 时，手动执行不受此限制
 */
if ($GLOBALS['user']->uid != 1) {
  $time_min = var_get('cron_min_time', 3600);

  if ($time_min && $timestamp > ($_SERVER['REQUEST_TIME'] - $time_min)) {
    echo 'byebye';
    exit;
  }
}

set_time_limit(600);

// 读取任务列表，每次最多 100 条
if ($fetch = db_query('SELECT * FROM {cron} WHERE status = 0 
ORDER BY weight ASC, cid ASC', NULL, array('limit' => 100))) {
  foreach ($fetch as $o) {
    if (!$o->data) continue;
    
    $data = unserialize($o->data);
    
    if ($data['includes']) {
      foreach ($data['includes'] as $filepath) {
        include_once $filepath;
      }
    }
    if (function_exists($data['func']) && call_user_func_array($data['func'], $data['args'])) {
      db_exec('UPDATE {cron} SET status = 1 WHERE cid = ?', array($o->cid));
      if ($data['success'] && function_exists($data['success'])) {
        call_user_func_array($data['success'], $data['args']);
      }
    }
    
  }
}

// 触发 hook_cron()

module_invoke_all('cron', $timestamp);

// 写入运行时间
var_set('cron_last_time', time());

// 写入日志
dd_log('cron', t('system', '成功运行了计划任务'));

echo 'ok!';
