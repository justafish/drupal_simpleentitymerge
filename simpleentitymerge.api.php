<?php

/**
 * Get the unique ID name for an entity.
 *
 * @return array
 */
function hook_simpleentitymerge_ids() {
  return array(
    'bean' => 'bid',
    'user' => 'uid',
  );
}
