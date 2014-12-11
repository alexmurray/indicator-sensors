((c-mode . ((eval . (let ((includes (split-string (replace-regexp-in-string "-I" "" (shell-command-to-string "pkg-config --cflags-only-I gtk+-3.0 glib-2.0 libpeas-1.0 appindicator3-0.1"))))
                          (definitions '("HAVE_CONFIG_H")))
                      (setq includes (append includes (list (concatenate 'string (file-name-directory
                                                                                  (let ((d (dir-locals-find-file ".")))
                                                                                    (if (stringp d) d (car d)))) "/indicator-sensors"))))
                      (setq flycheck-gcc-include-path includes)
                      (setq flycheck-clang-include-path includes)
                      (setq flycheck-gcc-definitions definitions)
                      (setq flycheck-clang-definitions definitions)
                      (setq company-clang-arguments (append
                                                     (mapcar
                                                      #'(lambda (s) (concat "-I" s))
                                                      includes)
                                                     (mapcar
                                                      #'(lambda (s) (concat "-D" s))
                                                      definitions))))))))
