(define-module (ui simple ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/simple/list_curr_usbs" ))
)

; list usbguard rules
(define (ls_guard_rules)
    (form-update-enum "list_hash_rules" (woo-list "/simple/list_rules" 'level "hash"))
    (form-update-enum "list_vidpid_rules" (woo-list "/simple/list_rules" 'level "vid_pid"))
    (form-update-enum "list_interface_rules" (woo-list "/simple/list_rules" 'level "interface"))
     (form-update-enum "list_unsorted_rules" (woo-list "/simple/list_rules" 'level "non-strict"))   
)

; get udev rules filenames
(define (udev_rules_check)
    (form-update-enum "suspicious_udev_files" (woo-list "/simple/check_config_udev"))
)

; remove first element from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))
; get value by key from associative  
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))

; get status
; usbguard: "OK" or "BAD"
; udev:     "OK" or BAD 
(define (config_status_check)
   (let ((status  (removeFirstElement (woo-read "/simple/config_status")) ))
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
                      ;      (form-update-activity "checkbox_use_control" #f)
                        )
                     )
                )    
           ) ; endif
           
           ; set checkbox checked if usbguard is active 
           (form-update-value "checkbox_use_control_hidden" 
                    (string=? "ACTIVE" (get-value 'usbguard_active status)))
           ;show allowed users and groups
           (form-update-value "usbguard_users" (get-value 'allowed_users status)) 
           (form-update-value "usbguard_groups" (get-value 'allowed_groups status)) 

   )  ; //let
) ; // config_status check

; unblock the selected device 
 (define (allow_device)
    (let ((  status  (woo-read-first "/simple/usb_allow" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (ls_usbs)
            (woo-throw  "Error while trying to unblock selected device")
        )
    ); let
 )

 ;block selected device
 (define (block_device)
     (let ((  status  (woo-read-first "/simple/usb_block" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (ls_usbs)
            (woo-throw  "Error while trying to block selected device")
        )
    ); let
 )

(define (init)
  (config_status_check)
  (ls_guard_rules)
  (form-bind "btn_prsnt_scan" "click" ls_usbs)
  (form-bind "btn_prsnt_dev_add" "click" allow_device)
  (form-bind "btn_prsnt_dev_block" "click" block_device)
)