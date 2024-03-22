(define-module (ui usbguard ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/usbguard/list_curr_usbs" ))
    (js "CatchDeviceSelection")
)

; list usbguard rules
(define (ls_guard_rules)
    (form-update-enum "list_hash_rules" (woo-list "/usbguard/list_rules" 'level "hash"))
    (form-update-enum "list_vidpid_rules" (woo-list "/usbguard/list_rules" 'level "vid_pid"))
    (form-update-enum "list_interface_rules" (woo-list "/usbguard/list_rules" 'level "interface"))
    (form-update-enum "list_unsorted_rules" (woo-list "/usbguard/list_rules" 'level "non-strict"))   
)

; get udev rules filenames
(define (udev_rules_check)
    (form-update-enum "suspicious_udev_files"  (woo-list "/usbguard/check_config_udev"))
)

; remove first element from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))
; get value by key from associative  
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))
; print object as string
(define (object->string obj)
  (call-with-output-string
    (lambda (port)
      (write obj port))))


; get status
; usbguard: "OK" or "BAD"
; udev:     "OK" or BAD 
(define (config_status_check)
   (let ((status  (removeFirstElement (woo-read "/usbguard/config_status")) ))
   ; (woo-error (object->string status))
           ; if udev=OK
           (if (string=? "OK" (get-value 'udev status)) 
                (begin
                    (form-update-visibility "udev_status_ok" #t)
                    (form-update-visibility "udev_config_warnings" #f)
                    (form-update-visibility "suspicious_udev_files" #f)
                    (form-update-visibility "udev_status_warnig" #f)
                )
               ; else
               ( begin
                    (form-update-visibility "udev_status_ok" #f)
                    (form-update-visibility "udev_status_warnig" #t)
                    (form-update-visibility "udev_config_warnings" #t)
                    (form-update-visibility "suspicious_udev_files" #t)
                    (udev_rules_check) 
               )
              
           ) ;endif

           ;if usbguard=OK 
           (if (string=? "OK" (get-value 'usbguard status)) ; 
                (begin
                    (form-update-visibility "guard_status_ok" #t)
                    (form-update-visibility "guard_status_bad" #f)
                    (form-update-visibility "list_prsnt_devices" #t)
                    (form-update-visibility "usb_buttons" #t)
                    (ls_usbs)
                )
                ;else usbguard not totally fine
                (begin
                     (if (string=? "ENABLED"  (get-value 'usbguard_enabled status)) 
                        (form-update-value "usbguard_enabled" (_ "Autorun is ENABLED"))
                        (form-update-value "usbguard_enabled" (_ "Autorun is DISABLED"))                                     
                     )
                     (if (string=? "ACTIVE"  (get-value 'usbguard_active status))
                         (form-update-value "usbguard_active" (_ "Process is ACTIVE") )
                         (form-update-value "usbguard_active" (_ "Process is STOPPED") )   
                     )                                     
                     (form-update-visibility "guard_status_ok" #f)
                     (form-update-visibility "guard_status_bad" #t)
                     ; if usbguard prosess is active
                     (if (string=? "ACTIVE" (get-value 'usbguard_active status))
                        (begin
                            (form-update-visibility "list_prsnt_devices" #t)
                            (ls_usbs)
                        ) 
                        ; else hide list of devices
                        (begin
                            (form-update-visibility "list_prsnt_devices" #f)
                            (form-update-visibility "usb_buttons" #f)
                        )
                     )
                )    
           ) ; endif
           
           ; file permissions warning
           (if  (string=? "OK" (get-value 'config_files_permissions status))
                (form-update-visibility "warning_file_permissions" #f)
                (form-update-visibility "warning_file_permissions" #t)
           ) ; endif            

           ; disable block button if default policy is block
           (if (string=? "block" (get-value 'implicit_policy status))
                    (begin        
                        (form-update-activity "btn_prsnt_dev_block"  #f)
                        (form-update-activity "btn_prsnt_dev_add"  #t) 
                    )
                    (begin
                        (form-update-activity "btn_prsnt_dev_block"  #t)
                        (form-update-activity "btn_prsnt_dev_add"  #f)
                    )
           ) ; endif     


           ;show allowed users and groups
           (form-update-value "usbguard_users" (get-value 'allowed_users status)) 
           (form-update-value "usbguard_groups" (get-value 'allowed_groups status)) 
                     
           ;select a list type (white or black)
           (if (string=? "block" (get-value 'implicit_policy status))
               (form-update-value "hidden_list_type" "radio_white_list")
               (form-update-value "hidden_list_type" "radio_black_list") 
           )

           ; set checkbox checked if usbguard is active 
           (if (string=? "ACTIVE" (get-value 'usbguard_active status))             
                (begin
                    (form-update-value "checkbox_use_control_hidden" #t)
                    (form-update-value "presets_input_hidden" "manual_mode")
                )
                ;select a preset
                (begin
                    (form-update-value "checkbox_use_control_hidden" #f)                  
                    (form-update-value "presets_input_hidden" "put_connected_to_white_list")
                )
           )

   )  ; //let
) ; // config_status check

; unblock the selected device 
 (define (allow_device)
    (if (string? (form-value "list_prsnt_devices"))
        (let ((  status  (woo-read-first "/usbguard/usb_allow" 'usb_id  (form-value "list_prsnt_devices")) )) 
            (if   
                (equal? "OK" (woo-get-option status 'status))
                (update_after_rulles_applied)
                (woo-error (_ "Error while trying to unblock a selected device. You can unblock it by creating a hash rule or interface rule."))
            )
        ); let
    ) ; endif
 )

 ;block selected device
 (define (block_device)
    (if (string? (form-value "list_prsnt_devices"))    
        (let ((  status  (woo-read-first "/usbguard/usb_block" 'usb_id  (form-value "list_prsnt_devices")) )) 
            (if   
                (equal? "OK" (woo-get-option status 'status))
                (update_after_rulles_applied)
                (woo-error  (_ "Error while trying to block a selected device"))
            )
        ); let
    ); endif
 )


; get all changes with one string from fronted and send data to backend
(define (save_rules_handler)
    (let ((  response  (removeFirstElement (woo-read "/usbguard/apply_changes" 'changes_json  (form-value "hidden_manual_changes_data"))) ))
        (if 
            (string=? "OK" (get-value 'status response))
            (js "ValidationResponseCallback" (get-value 'ids_json response) )
            (woo-error (_ "An error occurred when starting the USB Guard with new rules. We recommend checking the rules and configuration files for correctness." ))
        )
    ); //let 
)

; send all changes to backend for validation
(define (validate_rules_handler)
    (let ((  response  (removeFirstElement (woo-read "/usbguard/validate_changes" 'changes_json  (form-value "hidden_manual_changes_data"))) ))
        (if 
            (string=? "OK" (get-value 'status response))
            (js "ValidationResponseCallback" (get-value 'ids_json response) )
            (woo-error (_ "An error occurred when starting the USB Guard with new rules. We recommend checking the rules and configuration files for correctness." ))
        )
    ); //let
)

(define (update_after_rulles_applied)
    (config_status_check)
    (ls_guard_rules)
    (ls_usbs)
)

(define (filter-lists lst field)
  (filter (lambda (sublist) (member field sublist)) lst))

(define (upload_rules_callback)
    (let ((
        response  (removeFirstElement (woo-read "/usbguard/rules_upload" 'upload_rules (form-blob "file_input"))) 
         ))  
       (if  (string=? "OK" (get-value 'status response))
            (js "AddRulesFromFile" (get-value 'response_json response) )
            (begin
                (js "ClearFileInput")
                (if (string=? "ERROR_EMPTY" (get-value 'status response))
                    (js "alert" (_ "An empty csv file. Parsed 0 rules."))
                    (js "alert" (_ "An error occured while parsing csv file"))
                )                
            )            
       ) 
    ) ; let
)

; validation finished signal from js
(define (validation_finished)
    (form-update-activity "save_rules"  #t)
    (form-update-activity "validate_rules"  #f)
)
; some changes where made - validation needed
(define (validation_needed)
    (form-update-activity "save_rules"  #f)
    (form-update-activity "validate_rules"  #t)
)

(define (init)
  (validation_needed)  
  (form-bind "validate_rules" "validation_finished" validation_finished)  
  (form-bind "validate_rules" "validation_needed" validation_needed)
  (config_status_check)
  (ls_guard_rules)
  (form-bind "btn_prsnt_scan" "click" ls_usbs)
  (form-bind "btn_prsnt_dev_add" "click" allow_device)
  (form-bind "btn_prsnt_dev_block" "click" block_device)
  (form-bind "hidden_manual_changes_data" "rules_json_validation" validate_rules_handler) ;save changes event
  (form-bind "hidden_manual_changes_data" "rules_json_ready" save_rules_handler) ;save changes event
  (form-bind "hidden_manual_changes_response" "rules_applied" update_after_rulles_applied)  
  (form-bind-upload "load_file_button" "data_ready" "file_input" upload_rules_callback )
)