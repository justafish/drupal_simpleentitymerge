<?php

function hook_simpleentitymerge_save() {
  return array(
    'bean' => 'bean_save',
    'user' => 'user_save',
  );
}
