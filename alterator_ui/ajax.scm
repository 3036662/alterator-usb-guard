(define-module (ui usbguard ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/usbguard/list_curr_usbs" ))
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
           ; if udev=OK
           (if (string=? "OK" (get-value 'udev status)) 
                (begin
                    (form-update-visibility "udev_status_ok" #t)
                    (form-update-visibility "udev_config_warinigs" #f)
                    (form-update-visibility "suspicious_udev_files" #f)
                    (form-update-visibility "udev_status_warnig" #f)
                )
               ; else
               ( begin
                    (form-update-visibility "udev_status_ok" #f)
                    (form-update-visibility "udev_status_warnig" #t)
                    (form-update-visibility "udev_config_warinigs" #t)
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
                     (form-update-value "usbguard_enabled" (get-value 'usbguard_enabled status) )
                     (form-update-value "usbguard_active" (get-value 'usbguard_active status))
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

           ; set checkbox checked if usbguard is active 
           (form-update-value "checkbox_use_control_hidden" 
                    (string=? "ACTIVE" (get-value 'usbguard_active status)))
           ;show allowed users and groups
           (form-update-value "usbguard_users" (get-value 'allowed_users status)) 
           (form-update-value "usbguard_groups" (get-value 'allowed_groups status)) 
           
           ;select a preset
           (form-update-value "presets_input_hidden" "manual_mode")
           ;select a list type (white or black)
           (if (string=? "block" (get-value 'implicit_policy status))
               (form-update-value "hidden_list_type" "radio_white_list")
               (form-update-value "hidden_list_type" "radio_black_list") 
           )

   )  ; //let
) ; // config_status check

; unblock the selected device 
 (define (allow_device)
    (let ((  status  (woo-read-first "/usbguard/usb_allow" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (update_after_rulles_applied)
            (woo-throw  "Error while trying to unblock selected device")
        )
    ); let
 )

 ;block selected device
 (define (block_device)
     (let ((  status  (woo-read-first "/usbguard/usb_block" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (update_after_rulles_applied)
            (woo-throw  "Error while trying to block selected device")
        )
    ); let
 )


; get all changes with one string from fronted and send data to backend
(define (save_rules_handler)
    (let ((  response  (removeFirstElement (woo-read "/usbguard/apply_changes" 'changes_json  (form-value "hidden_manual_changes_data"))) ))
        (if 
            (string=? "OK" (get-value 'status response))
            (form-update-value "hidden_manual_changes_response" (get-value 'ids_json response) )
            (woo-error "An error occured while processing new rules" )
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
            (woo-error "An error occured while parsing csv file")
       ) 
    ) ; let
)

(define (init)
  (config_status_check)
  (ls_guard_rules)
  (form-bind "btn_prsnt_scan" "click" ls_usbs)
  (form-bind "btn_prsnt_dev_add" "click" allow_device)
  (form-bind "btn_prsnt_dev_block" "click" block_device)
  (form-bind "hidden_manual_changes_data" "rules_json_ready" save_rules_handler) ;save changes event
  (form-bind "hidden_manual_changes_response" "rules_applied" update_after_rulles_applied)  
  (form-bind-upload "load_file_button" "data_ready" "file_input" upload_rules_callback )
)