$(document).ready(function () {

  $("#usbguard_enabled").bind('update-value change', function () {
    $("#span_usbguard_enabled").text( $("#usbguard_enabled").val());
  });

  $("#usbguard_active").bind('update-value change', function () {
    $("#span_usbguard_active").text( $("#usbguard_active").val());
  });

  // workaround to check and uncheck checkbox as usual input
  // state of check  can be changed from lisp 
  // with form-update-value for hidden input 
  $('#checkbox_use_control').change(function () {
    $('#validate_rules_button').trigger('validation_needed');
    if ($("#checkbox_use_control").attr('checked')) {
      $("#checkbox_use_control_hidden").val("#t");
    }
    else {
      $("#checkbox_use_control_hidden").val("#f");
    }
  });

  $('#checkbox_use_control_hidden').bind('update-value change', function () {
    if ($("#checkbox_use_control_hidden").val() === "#t") {
      $("#checkbox_use_control").attr('checked', true);
    }
    else {
      $("#checkbox_use_control").attr('checked', false);
    }
  });
  $('#checkbox_use_control_hidden').trigger('change');

  /*******************************************************/
  // catch a preset selection event
  // put current value to presets_input_hidden
  $('input[type=radio][name=presets]').change(function () {
    $('#validate_rules_button').trigger('validation_needed');
    if ($(this).is(':checked')) {
      if ($(this).val()=="radio_white_list" || $(this).val()=="radio_black_list"){
        ActivateManualModeButtons(true);
        $("#presets_input_hidden").val("manual_mode");
        $("#hidden_list_type").val($(this).val());
        return;             
      } 
      // if not manual mode - disable add and delete buttons
      else{
        ActivateManualModeButtons(false);
      }      
      $("#presets_input_hidden").val($(this).val());
    }
  });

  // set preset selection
  // set radiobutton if presets_input_hidden changes
  $('#presets_input_hidden').bind('update-value change', function () {
    $('input[type=radio][name=presets][value=' + $("#presets_input_hidden").val() + ']').attr('checked', true);
  });
  
  // preselect type-of-list
  $('#hidden_list_type').bind('update-value change', function () {
    if ($('#hidden_list_type').val()==="radio_white_list"){
        $('#preset_manual_white').attr('checked',true);
    }
    else {
        $('#preset_manual_black').attr('checked',true);
    }
  });

  $("#presets_input_hidden").trigger('change');
  $('#hidden_list_type').trigger('change');

  /*******************************************************/
  // delete strings from tables

  localStorage.removeItem("deletedFields");
  $("#delete_rules_from_hash_level").bind('click', "form_list_hash_rules", SaveDeleted);
  $("#delete_rules_from_vid_pid_level").bind('click', "form_list_vidpid_rules", SaveDeleted);
  $("#delete_rules_from_interface_level").bind('click', "form_list_interface_rules", SaveDeleted);
  $("#delete_rules_from_unsorted_level").bind('click', "form_list_unsorted_rules", SaveDeleted);


  /*******************************************************/
  // add strings to tables

  // add new hash-level rule
  const new_hash_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="hash"><span></td>' +
    '<td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_hash", new_hash_row, "#list_hash_rules");

  // add new vidpid-level rule
  const new_vidpid_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="vid"><span></td>' +
    '<td>--</td>' +
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="pid"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_vidpid", new_vidpid_row, "#list_vidpid_rules");

  // add new CC::SS::PP rule
  const new_interface_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="with_interface"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_interfase", new_interface_row, "#list_interface_rules");

  // add new rule to unsorted-rules
  const new_unsorted_rules_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="raw_rule"><span></td>';
  addRuleBehaviorAdd("#add_to_rules_unsorted", new_unsorted_rules_row, "#list_unsorted_rules");
  window.last_append_rule_id = 0;
  
  // on click "save rules"
  // read stored values, put them to hidden input, trigger "ready" event for it
  $("#save_rules_button").bind('click', function () {
    let deletedFields = localStorage.getItem('deletedFields');
    let rules_changes={
      preset_mode: $("#presets_input_hidden").val(),
      deleted_rules:  JSON.parse(deletedFields),
      appended_rules: collectAppendedRules(),
      run_daemon: $("#checkbox_use_control_hidden").val()==="#t" ? "true":"false",
      policy_type: $("#hidden_list_type").val() 
    }
    $("#hidden_manual_changes_data").val(JSON.stringify(rules_changes));
   // $(".manual_appended").remove();
    $("#hidden_manual_changes_data").trigger('rules_json_ready');
  });

  // on check rules clicked
  $("#validate_rules_button").bind('click', function () {
    let deletedFields = localStorage.getItem('deletedFields');
    let rules_changes={
      preset_mode: $("#presets_input_hidden").val(),
      deleted_rules:  JSON.parse(deletedFields),
      appended_rules: collectAppendedRules(),
      run_daemon: $("#checkbox_use_control_hidden").val()==="#t" ? "true":"false",
      policy_type: $("#hidden_list_type").val() 
    }
    $("#hidden_manual_changes_data").val(JSON.stringify(rules_changes));
    $("#hidden_manual_changes_data").trigger('rules_json_validation');
  });

 /**
  * file upload
  * Substitute a file with its content, encoded with base64
  * File is substituted because alterator can't handle loading file
  * with unicode-escaped symbols
  */
 $("#load_file_button").bind('click',function(){
  const fileInput = document.getElementById('file_input');
  if (fileInput.files.length > 0) {
    if(fileInput.files[0].size >1024000 ){
        alert($("#alert_huge_file").text());
        fileInput.value="";
    }
    if(fileInput.files[0].size ==0 ){
      alert($("#alert_empty_file").text());
      fileInput.value="";
    }
    const file = fileInput.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = function(event) {
            const fileContent = event.target.result;
            const encodedString = btoa(fileContent);
            let file_encoded = new File([encodedString], "encoded.csv", {
                    type: "text/plain",
                    lastModified: new Date()
                });
            let container = new DataTransfer();
            container.items.add(file_encoded); 
            fileInput.files = container.files;
            // console.log(encodedString);
            $("#load_file_button").trigger("data_ready");
        };
      reader.readAsBinaryString(file);
    }
  } 
 });

 // bind appended checkboxes to checkbox in headers of tables
 CatchTableHeaderCheckbok('list_hash_rules');
 CatchTableHeaderCheckbok('list_vidpid_rules');
 CatchTableHeaderCheckbok('list_interface_rules');
 CatchTableHeaderCheckbok('list_unsorted_rules');



}); // .ready


// show validation result in tables
function ValidationResponseCallback(data){
  $(".validator_appended").remove();
  if ( data==="") return;  
  let response=JSON.parse(data);
  if (response["STATUS"] ==="OK" && response["ACTION"]==="apply"){
      $(".manual_appended").remove();
      localStorage.removeItem("deletedFields");
      ActivateManualModeButtons(true);
      $("#hidden_manual_changes_response").trigger("rules_applied");
      return;
  }
  for (const el of response["rules_BAD"]){
      $('#'+el).css("border", "3px red solid");
  }
  for (const el of response["rules_OK"]){
      $('#'+el).css("border", "3px green solid");
  }
  CrossDeletedByBackend("list_hash_rules",response["rules_DELETED"]);
  CrossDeletedByBackend("list_vidpid_rules",response["rules_DELETED"]);
  CrossDeletedByBackend("list_interface_rules",response["rules_DELETED"]);
  CrossDeletedByBackend("list_unsorted_rules",response["rules_DELETED"]);
  // if some rules where appended by preset 
  if (response.hasOwnProperty('rules_PRESET')){
    //hash
    if (response['rules_PRESET'].hasOwnProperty('hash') &&
        Array.isArray(response['rules_PRESET']['hash'])){
        response['rules_PRESET']['hash'].forEach((el) => 
        {
          $("#list_hash_rules").append(
            '<tr class="validator_appended">'+
            '<td></td><td>--</td>'+
            '<td><span class="alterator-label">'+el['hash'] +'</span></td> '+
            '<td>'+el['description']+'</td>'+
            '<td>'+(el['target']=== 0 ? "allow" : "block") +'</td></tr>'
          );
        });
    }
    // interface
    if (response['rules_PRESET'].hasOwnProperty('interface') &&
        Array.isArray(response['rules_PRESET']['interface'])){
        response['rules_PRESET']['interface'].forEach((el) => 
        {
          $("#list_interface_rules").append(
            '<tr class="validator_appended">'+
            '<td></td><td>--</td>' +       
        '<td><span class="alterator-label">'+el['interface']+'</span></td>' +
        '<td>--</td><td>--</td>'+
        '<td>' + (el["target"] === 0 ? "allow" : "block") +'</td></tr>');          
        });
    }
    // vidpid
    if (response['rules_PRESET'].hasOwnProperty('vidpid') &&
        Array.isArray(response['rules_PRESET']['vidpid'])){
      response['rules_PRESET']['vidpid'].forEach((el) => 
      {
      $("#list_vidpid_rules").append(
        '<tr class="validator_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td></td><td>--</td>' +       
        '<td><span class="alterator-label">'+el['vid']+'</span></td>' +
        '<td>--</td>' +
        '<td><span class="alterator-label">'+el['pid']+'</span></td>' +
        '<td>--</td><td>--</td>'+
        '<td>' + (el["policy"] === 0 ? "allow" : "block") + '</td></tr>');   
      });
    }
    // raw
    if (response['rules_PRESET'].hasOwnProperty('raw') &&
        Array.isArray(response['rules_PRESET']['raw'])){
      response['rules_PRESET']['raw'].forEach((el) => 
      {
      $("#list_unsorted_rules").append(
        '<tr class="validator_appended">'+
        '<td></td><td>--</td>' + 
        '<td><span class="alterator-label">'+el['raw'] +'</span></td>' +
        '<td>' +(el["target"] === 0 ? "allow" : "block") + '</td></tr>');
      });
    }
  }
  // tell alterator to enable save button if everything is OK
  if (response["STATUS"] ==="OK"){
    $('#validate_rules_button').trigger('validation_finished');
  }
}


function ClearFileInput(){
  const fileInput = document.getElementById('file_input');
  fileInput.value="";
}

function AddRulesFromFile (data) {
  const rules_json=JSON.parse(data);
  //set policy ratiobuttons
  if (rules_json["STATUS"]==="error"){
    if (rules_json["ERR_MSG"]==="Conflicting rule targets"){
      alert($("#alert_conflicts_in_file").text());
      ClearFileInput();
      return;
    }
  }
  if (rules_json.hasOwnProperty('policy')){
    if (rules_json['policy'] === 0){
        $("#form_presets").val("radio_white_list"); 
        $("#hidden_list_type").val("radio_white_list");
    }
    else{
      $("#form_presets").val("radio_black_list"); 
      $("#hidden_list_type").val("radio_black_list");
    }
  }
  // fill the hash table
  if (rules_json.hasOwnProperty("hash_rules") && Array.isArray(rules_json["hash_rules"])){
    rules_json["hash_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_hash_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="hash" value=\''+
        rule["hash"]+
        '\'/><span></td>' +
        '<td>-</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });    
  };
  // fill the vidpid rules table
  if (rules_json.hasOwnProperty("vidpid_rules") && Array.isArray(rules_json["vidpid_rules"])){
    rules_json["vidpid_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_vidpid_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +       
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="vid" value=\''+
        rule["vid"]+
        '\'><span></td>' +
        '<td>--</td>' +
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="pid" value=\''+
        rule["pid"]+
        '\'><span></td>' +
        '<td>--</td><td>--</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });  
  };
  // fill the interface rule target
  if (rules_json.hasOwnProperty("interf_rules") && Array.isArray(rules_json["interf_rules"])){
    rules_json["interf_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_interface_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +       
        '<td><span class="alterator-label">'+
        '<input type="text" class="input_appended" name="with_interface" value=\''+
        rule["interface"]+
        '\' /><span></td>' +
        '<td>--</td><td>--</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });    
  };
  // fill the row rules table
  if (rules_json.hasOwnProperty("raw_rules") && Array.isArray(rules_json["raw_rules"])){
    rules_json["raw_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      alert (rule["raw"]);
      $("#list_unsorted_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' + 
        '<td><span class="alterator-label">'+
        '<input type="text" class="input_appended" name="raw_rule" value=\''+
        rule["raw"]+
        '\'><span></td>' +
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });
  };  
  bindCheckBox();
  $('#validate_rules_button').trigger('validation_needed');
  document.getElementById('file_input').value = '';
};


// add appending editable rule to a table
function addRuleBehaviorAdd(button_id, row_html, table_id) {
  $(button_id).bind('click', function () {
    $('#validate_rules_button').trigger('validation_needed');
    ++window.last_append_rule_id;
    $(table_id).append(
      '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'"><td><input class="select_appended" type="checkbox"></td>' +
      '<td>--</td>' +
      row_html + '<td class="appended_rule_target">' +
      ($("#hidden_list_type").val() === "radio_white_list" ? "allow" : "block") +
      '</td></tr>');
    bindCheckBox();
    // validation is needed after change
    $('.input_appended').bind('input',function(){
      if ($('#validate_rules_button').hasClass('ui-state-disabled')){
        $("#validate_rules_button").removeClass("ui-state-disabled");
        document.getElementById('validate_rules_button').disabled=false; 
        $("#save_rules_button").addClass("ui-state-disabled");
        document.getElementById('save_rules_button').disabled=true; 
      }
      });
  });
};


// bind appended change - add "selected" class
function bindCheckBox() {
  $('.select_appended').change(function () {
    if ($(this).is(':checked')) {
      $(this).closest('tr').addClass('selected');
    }
    else {
      $(this).closest('tr').removeClass('selected');
    }
  });
};


// collects appended rules from tables
function collectAppendedRules() {
  let appended_rules=[];
  // for each appended rule
  $(".manual_appended").each(function () {
    // get a type of rule
    let table_id = $(this).closest('table').attr('id');
    let target = $(this).find("td.appended_rule_target")[0];
    let tr_id = $(this).attr('id');
    // put all field values to array
    let inputs =$(this).find("input.input_appended");
    let fields =[];
    inputs.each(function(i,el){
      let name=$(el).attr('name');
      let val=$(el).val();
      fields.push({[name]:val});
    });
    // put rule to the result array (appended_rules)
    appended_rules.push({table_id:table_id,tr_id:tr_id,target:$(target).text(),fields_arr:fields});
  });
  return appended_rules;
}

// save deleted rules to local storage
function SaveDeleted(table_id) {
  let deletedFields = JSON.parse(localStorage.getItem('deletedFields')) || [];
  $('#' + table_id.data + " tr.selected").each(function () {
    let val = $(this).find("td > span[name='name']").text();
    if (val !== "") {
      deletedFields.push(val);
    }
    if ($(this).hasClass("manual_appended")){
      $(this).remove()
    }
    else{
      $('#validate_rules_button').trigger('validation_needed');
      $(this).addClass('crossed-out');
    }
  });
  localStorage.setItem('deletedFields', JSON.stringify(deletedFields));
}

// cross-out rules delete by preset
function CrossDeletedByBackend(table_id,deleted){
  $('#' + table_id + " tr").each(function () {
    let val = $(this).find("span[name='name']").text();
    if (val !=="" && deleted.includes(parseInt(val,10)) && !$(this).hasClass('crossed-out') ){
      $(this).addClass('crossed-out');
      $(this).css("border", "2px black solid");
    }
  });
}

// @param activate - bool true || false
function ActivateManualModeButtons(activate){
  let buttons = document.getElementsByClassName("manual_mode_button");
  if (activate){
    for (var i = 0; i < buttons.length; i++) {
      buttons[i].disabled = false;
      buttons[i].classList.remove("ui-state-disabled");
    }
  }
  else{
    for (var i = 0; i < buttons.length; i++) {
      buttons[i].disabled = true;
      buttons[i].classList.add("ui-state-disabled");          
    }
  }
}

function DisableManualModeButtons(){
  ActivateManualModeButtons(false);
}



// check all appended rules if checkbox in table header is checked
function CatchTableHeaderCheckbok(table_id){
  let checkboxTh = $("#"+table_id+" th input[type='checkbox']:first");
  if (checkboxTh.length>0){
    $(checkboxTh[0]).change(function (){
      if ($(this).is(':checked')) {
        $('#'+table_id+' '+'input.select_appended[type="checkbox"]').attr("checked", true);
        $('#'+table_id+' '+'input.select_appended[type="checkbox"]').trigger("change")
        
      }
      else {
        $('#'+table_id+' '+'input.select_appended[type="checkbox"]').attr("checked", false);
        $('#'+table_id+' '+'input.select_appended[type="checkbox"]').trigger("change");
      }
    });
  }
}

// disable block button if already blocked
// prevent blocking already block (and unblocking already unblocked)
function CatchDeviceSelection(){ 
 // enable block button
 if ($("#btn_prsnt_dev_block").hasClass("disabled_by_block_rule")){
  $("#btn_prsnt_dev_block").removeClass("disabled_by_block_rule");
  $("#btn_prsnt_dev_block").removeClass("ui-state-disabled");
  document.getElementById('btn_prsnt_dev_block').disabled=false; 
}
// enable allow button
 if ($("#btn_prsnt_dev_add").hasClass("disabled_by_allow_rule")){
  $("#btn_prsnt_dev_add").removeClass("disabled_by_allow_rule");
  $("#btn_prsnt_dev_add").removeClass("ui-state-disabled");
  document.getElementById('btn_prsnt_dev_add').disabled=false;
}

// bind click
 $('#devices_list_table tr').bind('click',(function() {
  var status = $(this).find('span[name="label_prsnt_usb_status"]').text();
  if (status==="allow"){
    // disable allow button
    if (!$("#btn_prsnt_dev_add").hasClass("ui-state-disabled") &&  document.getElementById('btn_prsnt_dev_add').disabled==false){
      $("#btn_prsnt_dev_add").addClass("ui-state-disabled");
      $("#btn_prsnt_dev_add").addClass("disabled_by_allow_rule");
      document.getElementById('btn_prsnt_dev_add').disabled=true;     
    }
    // enable block button
    if ($("#btn_prsnt_dev_block").hasClass("disabled_by_block_rule")){
      $("#btn_prsnt_dev_block").removeClass("disabled_by_block_rule");
      $("#btn_prsnt_dev_block").removeClass("ui-state-disabled");
      document.getElementById('btn_prsnt_dev_block').disabled=false; 
    }
  }
  if (status==="block"){
    // enable allow button
    if ($("#btn_prsnt_dev_add").hasClass("disabled_by_allow_rule")){
      $("#btn_prsnt_dev_add").removeClass("disabled_by_allow_rule");
      $("#btn_prsnt_dev_add").removeClass("ui-state-disabled");
      document.getElementById('btn_prsnt_dev_add').disabled=false;
    }
    //disable block button
    if (!$("#btn_prsnt_dev_block").hasClass("ui-state-disabled") && document.getElementById('btn_prsnt_dev_block').disabled==false){
      $("#btn_prsnt_dev_block").addClass("ui-state-disabled");
      $("#btn_prsnt_dev_block").addClass("disabled_by_block_rule");
      document.getElementById('btn_prsnt_dev_block').disabled=true;      
    }
    
  }
 }));
}

function UpdateTextArrea(id,data){
  var textareaElement = $('textarea[name="'+id+'"]');
  if (textareaElement.length > 0) {
    textareaElement.val(data);
  }
}