<?php
/**
 * @file
 * fg_feature_charts.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function fg_feature_charts_ctools_plugin_api($module = NULL, $api = NULL) {
  if ($module == "strongarm" && $api == "strongarm") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_node_info().
 */
function fg_feature_charts_node_info() {
  $items = array(
    'charts' => array(
      'name' => t('Charts'),
      'base' => 'node_content',
      'description' => '',
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  drupal_alter('node_info', $items);
  return $items;
}
