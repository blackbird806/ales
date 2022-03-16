;(defun square (x)
;	(* x x)
;	)
;
;(print (square 5))
;
;(defmacro test (cond body)
;	(if (macroexpand cond)
;		(macroexpand body))
;	)
;
;(test (eq 1 1) (print "yay"))
;
;(defmacro decl-fn ()
; 	(defun cube (x)
; 		(* x x x)
; 		)
; 	)
; 
;(defmacro inc (x)
;	(set (macroexpand x) (+ (macroexpand x) 1))
;	)
;
;(decl-fn)
;(set b 10)
;(inc b)
;(inc b)
;(inc b)
;(print b)

(print (quote (1 2 3)))
(print (quote abc))