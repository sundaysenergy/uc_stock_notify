<?php
// $Id$

/**
 * @file
 * This file contains all the Conditional Action Hooks for uc_stock_notify.
 */

/**
 * Implementation of hook_ca_entity().
 */
function uc_stock_notify_ca_entity() {
  $entities = array();
  $entities['model'] = array(
    '#title' => t('Product SKU'),
    '#type' => 'string',
  );
  return $entities;
}

/**
 * Implementation of hook_ca_trigger().
 */
function uc_stock_notify_ca_trigger() {
  $triggers['uc_stock_notify_restocked'] = array(
    '#title' => t('Model has been restocked'),
    '#category' => t('Stock'),
    '#arguments' => array(
      'model' => array(
        '#entity' => 'model',
        '#title' => t('Product SKU'),
      ),
    ),
  );
  return $triggers;
}

/**
 * Implementation of hook_ca_predicate().
 */
function uc_stock_notify_ca_predicate() {
  $predicates['uc_stock_notify_send_notification'] = array(
    '#title' => t("Notify customers when product comes back into stock"),
    '#trigger' => 'uc_stock_notify_restocked',
    '#class' => 'uc_stock_notify',
    '#status' => 1,
    '#actions' => array(
      array(
        '#name' => 'uc_stock_notify_action_email',
        '#title' => t('Notify customers by email when product comes back into stock'),
        '#argument_map' => array(
          'model' => 'model',
        ),
        '#settings' => array(
          'from' => uc_store_email_from(),
          'subject' => t('[store-name]: [title] back in stock'),
          'message' => t("This message has been sent to let you know that <a href=\"[node-url]\">[title]</a> is now back in stock.\n\nThanks,\n\n[store-name]\n[store-url]"),
          'format' => 1,
        ),
      ),
    ),
  );
  return $predicates;
}

/**
 * Implementation of hook_action().
 */
function uc_stock_notify_ca_action() {
  $actions['uc_stock_notify_action_email'] = array(
    '#title' => t('Notify customers by email when product comes back into stock'),
    '#callback' => 'uc_stock_notify_action_email',
    '#arguments' => array(
      'model' => array(
        '#entity' => 'model',
        '#title' => t('Product SKU'),
      ),
    ),
    '#category' => t('Stock'),
  );
  return $actions;
}

/**
 * Send notifications to selected users.
 */
function uc_stock_notify_action_email($model, $settings) {
  $nid = db_result(db_query("SELECT n.nid FROM {uc_products} p INNER JOIN {node} n ON p.vid = n.vid WHERE p.model = '%s'", $model));
  $node = node_load($nid);

  $settings['replacements'] = array(
    'global' => NULL,
    'node' => $node,
    'product' => uc_product_load($node),
  );

  $result = db_query("SELECT sn.uid, u.mail, sn.mail anon_mail FROM {uc_stock_notify} sn LEFT JOIN {users} u ON sn.uid = u.uid WHERE sku = '%s'", $model);
  $sent = 0;
  while ($row = db_fetch_array($result)) {
    $mail = $row['uid'] ? $row['mail'] : $row['anon_mail'];
    drupal_mail('uc_order', 'action-mail', $mail, uc_store_mail_recipient_language($mail), $settings, $settings['from']);
    $sent++;
  }

  if ($sent) {
    // Only send notifications once about an item being restocked.
    db_query("DELETE FROM {uc_stock_notify} WHERE sku = '%s'", $model);
    drupal_set_message(t('@count back in stock notice(s) sent for @model.', array('@count' => $sent, '@model' => $model)));
  }
}

/**
 * Configuration form for stock notify action.
 */
function uc_stock_notify_action_email_form($form_state, $settings = array()) {
  $form = ca_build_email_form($form_state, $settings, array('global', 'node', 'product'));
  unset($form['addresses']);

  return $form;
}
