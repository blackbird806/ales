(defun square (x)
	(* x x)
	)

(print (square 5))

(defmacro test (cond body)
	(if (macroexpand cond)
		(macroexpand body))
	)

(test (eq 1 0) (print "yay"))