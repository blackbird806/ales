;(defun square (x)
;	(* x x)
;	)
;
;(print (square 5))
;
(defmacro test (cond body)
	(if (macroexpand cond)
		(macroexpand body))
	)

(test (eq 1 1) (print "yay"))

(defmacro decl-fn ()
 	(defun cube (x)
 		(* x x x)
 		)
 	)
 
(decl-fn)
(print (cube 5))

