<?php
/**
 * @file
 * Integrates external auth code from Drupal 7 core, removed in Backdrop.
 */

/**
 * Fetches a user object based on an external authentication source.
 *
 * @param string $authname
 *   The external authentication username.
 *
 * @return
 *   A fully-loaded user object if the user is found or FALSE if not found.
 */
function shib_auth_external_load($authname) {
  $uid = db_query("SELECT uid FROM {shib_auth_authmap} WHERE authname = :authname", array(
    ':authname' => $authname,
  ))
    ->fetchField();
  if ($uid) {
    return user_load($uid);
  }
  else {
    return FALSE;
  }
}

/**
 * Helper function for authentication modules.
 *
 * Either logs in or registers the current user, based on username. Either way,
 * the global $user object is populated and login tasks are performed.
 */
function shib_auth_external_login_register($name, $module) {
  $account = shib_auth_external_load($name);
  if (!$account) {

    // Register this new user.
    $userinfo = array(
      'name' => $name,
      'pass' => user_password(),
      'init' => $name,
      'status' => 1,
      'access' => REQUEST_TIME,
    );
    $account = user_save(backdrop_anonymous_user(), $userinfo);

    // Terminate if an error occurred during user_save().
    if (!$account) {
      backdrop_set_message(t("Error saving user account."), 'error');
      return;
    }
    shib_auth_set_authmaps($account, array(
      "authname_{$module}" => $name,
    ));
  }

  // Log user in.
  $form_state['uid'] = $account->uid;
  user_login_submit(array(), $form_state);
}

/**
 * Maps external usernames to user ids in the users table.
 *
 * Save mappings of which external authentication module(s) authenticated
 * a user.
 *
 * @param $account
 *   A user object.
 * @param $authmaps
 *   An associative array with a compound key and the username as the value.
 *   The key is made up of 'authname_' plus the name of the external authentication
 *   module.
 * @see shib_auth_external_login_register()
 */
function shib_auth_set_authmaps($account, $authmaps) {
  foreach ($authmaps as $key => $value) {
    $module = explode('_', $key, 2);
    if ($value) {
      db_merge('shib_auth_authmap')
        ->key(array(
        'uid' => $account->uid,
        'module' => $module[1],
      ))
        ->fields(array(
        'authname' => $value,
      ))
        ->execute();
    }
    else {
      db_delete('shib_auth_authmap')
        ->condition('uid', $account->uid)
        ->condition('module', $module[1])
        ->execute();
    }
  }
}

/**
 * Discover which external authentication module(s) authenticated a username.
 *
 * @param $authname
 *   A username used by an external authentication module.
 * @return
 *   An associative array with module as key and username as value.
 */
function shib_auth_get_authmaps($authname = NULL) {
  $authmaps = db_query("SELECT module, authname FROM {shib_auth_authmap} WHERE authname = :authname", array(
    ':authname' => $authname
  ))->fetchAllKeyed();
  return count($authmaps) ? $authmaps : 0;
}
