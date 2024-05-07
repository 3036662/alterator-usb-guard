(define-module (ui usbmount ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (object->string obj)
  (call-with-output-string
    (lambda (port)
      (write obj port))))    
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))

; remove first element from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))

(define (ls-devices)
  (form-update-enum "list_prsnt_devices" (woo-list "/usbmount/list_block" ))
)

(define (ls-rules)
 (js "UpdateRulesList" (car(removeFirstElement (woo-read "/usbmount/list-devices"))))
)

(define (save-rules)
;(woo-error (object->string   (car(removeFirstElement(woo-read "/usbmount/save_rules" 'data (form-value "new_rules_data")))) ))
     (if (string=? "OK" (caar(removeFirstElement(woo-read "/usbmount/save_rules" 'data (form-value "new_rules_data")))))  
      (ls-rules)
      (woo-error (_ "Save rules operation FAILED"))
     ); //if
)

(define (update_ui)
        (js "SetHealthStatus" "OK")
        (js "SetUsersAndGroups" (car(removeFirstElement(woo-read "/usbmount/get_users_groups"))))
        (ls-rules)
        (ls-devices)
)

(define (run_service)
    (if (string=? "OK" (caar(removeFirstElement(woo-read "/usbmount/run"))))
      (update_ui)
      (woo-error (_ "Error starting the service"))
    )
)

(define (stop_service)
    (if (string=? "OK" (caar(removeFirstElement(woo-read "/usbmount/stop"))))
      (js "SetHealthStatus" "DEAD")
      (woo-error (_ "Error stop the service"))
    )
)

(define (load_recent_log)
 ; (woo-error (object->string (caar(removeFirstElement(woo-read "/usbmount/read_log" 'page "0" 'filter "")))))
  (js "SetLogData"  (caar(removeFirstElement(woo-read "/usbmount/read_log" 'page "0" 'filter ""))))
)

(define (update_log)
  (js "SetLogData"  (caar(removeFirstElement(woo-read "/usbmount/read_log" 'page (form-value "current_page")  'filter (form-value "log_filter") ) )))
)


(define (init)
 (let ((health (caar(removeFirstElement(woo-read "/usbmount/health" ))) )) 
    (js "InitUi" health)
    (if (string=? "OK" health)
      (update_ui)
    ) 
  )  
 (load_recent_log)
 (form-bind "btn_prsnt_scan" "click" ls-devices)
 (form-bind "btn_save" "rules_data_ready" save-rules)
 (form-bind "btn_save_status" "btn_run_service" run_service)
 (form-bind "btn_save_status" "btn_stop_service" stop_service)
 (form-bind "current_page" "page_change" update_log)
)