<?php

/**
 * Populates the member object with the values from the form submission.
 * Note: $member param is nullable. In this case a new member class instance is created with the default settings.
 *
 * @return populated member object
 */
function egbolig_webform_populate_member($member, $form_state) {
  $v = $form_state['values']['submitted'];
  $tree = $form_state['webform']['component_tree']['children'];

  //check if member is empty, create a new member and populate some default fields
  if ($member == NULL) {
    $member = new stdClass();
    //TODO: check default fields
    $member->Password = DEFAULT_PASSWORD;
    $member->OfferByEmail = DEFAULT_OFFER_BY_EMAIL;
    $member->CprNo = $form_state['cpr'];
    $member->MemberNo = $form_state['cpr'];
  }
  $member->Email = $v[_egbolig_webform_get_webform_cid($tree, 'email')];
  $member->Name = $v[_egbolig_webform_get_webform_cid($tree, 'name')];
  $member->Address = $v[_egbolig_webform_get_webform_cid($tree, 'addresse')];
  $member->HomePhone = $v[_egbolig_webform_get_webform_cid($tree, 'phone_nr')];
  $member->CoAddress = $v[_egbolig_webform_get_webform_cid($tree, 'c_o_addresse')];
  $member->MobilePhone = $v[_egbolig_webform_get_webform_cid($tree, 'telefonnummer_mobil')];
  $member->PostalCodeCity = $v[_egbolig_webform_get_webform_cid($tree, 'post_nr___by')];
  $member->Country = DEFAULT_COUNTRY;
  $member->Adults = 1 + $v[_egbolig_webform_get_webform_cid($tree, 'has_partner')][0]; //with or without the partner

  if (empty($v[_egbolig_webform_get_webform_cid($tree, 'has_children')][0])) {
    $member->Children = 0;
  }
  $education_name_cid = _egbolig_webform_get_webform_cid($tree, 'education_name');
  $member->Education = isset($v[$education_name_cid])? $v[$education_name_cid] : NULL;
  $member->EducationStart = !empty($v[_egbolig_webform_get_webform_cid($tree, 'education_start_date')]) ? date("Y-m-d", strtotime(str_replace('/', '.', $v[_egbolig_webform_get_webform_cid($tree, 'education_start_date')]))) : NULL;
  $member->EducationEnd = !empty($v[_egbolig_webform_get_webform_cid($tree, 'education_end_date')]) ? date("Y-m-d", strtotime(str_replace('/', '.', $v[_egbolig_webform_get_webform_cid($tree, 'education_end_date')]))) : NULL;
  $member->EducationExpectedEnd = !empty($v[_egbolig_webform_get_webform_cid($tree, 'education_end_date')]) ? date("Y-m-d", strtotime(str_replace('/', '.', $v[_egbolig_webform_get_webform_cid($tree, 'education_end_date')]))) : NULL;
  $education_documentation_received_cid = _egbolig_webform_get_webform_cid($tree, 'education_doc');
  $member->EducationDocumentationReceived = isset($v[$education_documentation_received_cid])? ($v[$education_documentation_received_cid] > 0) : FALSE; //TODO: not working with WS
  $educational_institution_cid = _egbolig_webform_get_webform_cid($tree, 'institution_name');
  $member->EducationalInstitution = isset($v[$educational_institution_cid])? $v[$educational_institution_cid] : NULL;
  $member->MaximumRent = $v[_egbolig_webform_get_webform_cid($tree, 'husleje_max')];
  $member->Pet = $v[_egbolig_webform_get_webform_cid($tree, 'husdyr')];
  $member->Criteria1 = $v[_egbolig_webform_get_webform_cid($tree, 'status')];
  if ($v[_egbolig_webform_get_webform_cid($tree, 'status')] == 11) {
    $member->Criteria2 = $v[_egbolig_webform_get_webform_cid($tree, 'arsog')];
  }


  //$member->Pet TODO: fix the Pets list

  return $member;
}

/**
 * checks if child added to the member
 */
function egbolig_webform_is_child_exists($child_cpr, $childrenList) {
  foreach ($childrenList as $child) {
    if ($child['Cpr4'] == substr($child_cpr, -4) && $child['Cpr6'] == substr($child_cpr, 0, 6)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * remove child from the children lit
 */
function egbolig_webform_remove_child_from_list($child_cpr, &$childrenList) {
  for ($i = 0; $i <= count($childrenList); $i++) {
    if ($childrenList[$i]['Cpr4'] == substr($child_cpr, -4) && $childrenList[$i]['Cpr6'] == substr($child_cpr, 0, 6)) {
      unset($childrenList[$i]);
      break;
    }
  }

  reset($childrenList);
}

/**
 * Returns min and max room from wish list as array values 0 => minRoom and 1 => maxRoom
 * if values for minRoom or maxRoom could not be found, null is returned in the same manner
 *
 * @param $member
 */
function egbolig_webform_get_wishes_rooms($member) {
  $wishes = egbolig_webform_get_wishes($member);

  $minRoom = NULL;
  $maxRoom = NULL;
  foreach($wishes as $wish) {
    if (is_null($maxRoom) || $wish->Room > $maxRoom) {
      $maxRoom = $wish->Room;
    }

    if (is_null($minRoom) || $wish->Room < $minRoom) {
      $minRoom = $wish->Room;
    }
  }

  return array(
    0 => $minRoom,
    1 => $maxRoom
  );
}

/**
 * Returns the webform cid based on the form key, performs a recursive search
 *
 * @param $tree
 * @param $form_key
 * @return mixed
 */
function _egbolig_webform_get_webform_cid($tree, $form_key) {
  foreach ($tree as $child) {
    if ($child['form_key'] === $form_key) {
      return $child['cid'];
    }
    else {
      if (isset($child['children'])) {
        $cid = _egbolig_webform_get_webform_cid($child['children'], $form_key);
        if ($cid) {
          return $cid;
        }
      }
    }
  }
}

/**
 * This function will return the number as string with the desired legth.
 * It is useful since EGBolig is cutting the numbers starting with zeros.
 *
 * @param $number
 * @param $desired_length
 */
function _egbolig_webform_get_number_fixed_length($number, $desired_length) {
  return sprintf("%'.0" . $desired_length . "d", $number);
}
