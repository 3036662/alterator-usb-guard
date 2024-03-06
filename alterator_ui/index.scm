(document:surround "/std/frame")

; remove first element from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))

; get value by key from associative  
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))

(define (object->string obj)
  (call-with-output-string
    (lambda (port)
      (write obj port))))

(define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/usbguard/list_curr_usbs" ))
)

(define (config_status_check)
   (let ((status  (removeFirstElement (woo-read "/usbguard/config_status")) ))
      (format #t "debug-message:obj1=~S" (get-value 'usbguard_enabled status))
       ; usbguard status check
       (if (string=? "OK" (get-value 'usbguard status)) 
          (begin 
             (form-update-value "usb_guard_status" "UsbGuard service ... ACTIVE") 
             (form-update-visibility "list_prsnt_devices" #t)
             (form-update-visibility "usb_buttons" #t)
          )
          ;  else (usbguard is not totally ok)
          ( begin
            (let ((usbguard_service_status "UsbGuard service ... Autorun is "))               
                 (set! usbguard_service_status (string-append usbguard_service_status (get-value 'usbguard_enabled status)))
                 (set! usbguard_service_status (string-append usbguard_service_status " Process is "))    
                 (set! usbguard_service_status (string-append usbguard_service_status (get-value 'usbguard_active status)))    
                 (form-update-value "usb_guard_status" usbguard_service_status) 
            
            ) ; //let        
            (if (string=? "ACTIVE" (get-value 'usbguard_active status))    
               (begin
                  (form-update-visibility "list_prsnt_devices" #t)
                  (form-update-visibility "usb_buttons" #t)
                  (ls_usbs)
               ) 
               ; else hide list of devices
               (begin
                            (form-update-visibility "list_prsnt_devices" #f)
                            (form-update-visibility "usb_buttons" #f)
               )
            ) ; endif
          )
       ) ; endif
       ; udev status check
       (if (string=? "OK" (get-value 'udev status))
            (begin
               (form-update-value "udev_rules_status" "Udev rules ... OK")
               (form-update-visibility "udev_warnings" #f)            
            )
            ; else
            ( begin
               (form-update-value "udev_rules_status" "Udev rules ... WARNING")
               (form-update-visibility "udev_warnings" #t)    
               (form-update-enum "list_suspicious_files"  (woo-list "/usbguard/check_config_udev")) 
            )
       )      
   ) ; let
)



(gridbox columns "2;96;2"
   (spacer)
   (groupbox title "Service status"
      (label name "usb_guard_status")
      (label name "udev_rules_status")
      (spacer)
      (vbox name "udev_warnings"
         (label text "Suspicious files were found in udev config folders.")
         (label text "It can cause an undefined behavior.")
         (label text "Please double-check before configuring.")
         (listbox name "list_suspicious_files"
            columns 1
            header (vector (_ "Filename"))
            row '#(
                  (name . "")
                  ) 
         )      
      )
   )
   (spacer)
   (spacer colspan 3)

   (spacer) 
   (vbox
      (listbox name "list_prsnt_devices"
      columns 10 
      header (vector (_ "N") (_ "Port") (_ "CC:SS:PP") (_ "VID") (_ "Vendor") (_ "PID") (_ "Product") (_ "SN") (_ "Hash") (_ "Status"))      
      row '#(
               (name . "") 
               (label_prsnt_usb_port . "") 
               (label_prsnt_usb_class . "") 
               (label_prsnt_usb_vid . "") 
               (label_prsnt_usb_vendor . "") 
               (label_prsnt_usb_pid . "")             
               (label_prsnt_usb_name . "")
               (label_prsnt_usb_serial . "")
               (label_prsnt_usb_hash . "")  
               (label_prsnt_usb_status . "") 
            )
      )
      ; buttons 
      (gridbox "70;30" name "usb_buttons"
         (spacer colspan 1)
         (hbox align "right"  
            (button text "Scan plugged-in USB devices")
            (button text "Allow device")
            (button text "Block device")
         )
      )
   ) ;//vbox
   (spacer) 
   ;;
   
)




(define (ui-init)
    (ls_usbs)
    (config_status_check)
    ;(format #t "debug-message:obj1=~S" (removeFirstElement (woo-list "/usbguard/list_curr_usbs" )) ) 
)

(document:root (when loaded 
    (ui-init)

))