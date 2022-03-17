
(defmacro inc (x)
	(print x) ; print 10
	(set x (+ x 1))
	)

(defmacro test (cond code)
	(if cond
		code
		)
	)
(test (eq 1 1) (print "yay"))

(set b 10)
(inc b)
(inc b)
(inc b)
(print b)