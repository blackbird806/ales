
(set my-tree (
	(set value 5)
	(set left (set value 12))
	(set right (set value 7))
	)

(defun insert-child (tree child)
		(set (. tree left) child)
	)

(insert-child (. my-tree left) 
	(set node ((set value 5))
		)
	)

(set print-value-st (
		(defun print-value ()
			(println (. self value)) ; self refer to the current statement
			)

		(defun print-value-mul (a)
			(println (. self (* value a)))
			)

		(set print-lambda 
			(lambda (* value value))
			)
		)
	)

(set-meta-statement my-tree print-value-st)

(println (. my-tree value)) // 5
(println (. (. my-tree left) value)) // 12
((. my-tree print-value-mul) 12) // 5 * 12

(defmacro call (st fn ...)
	((. st fn) ...)
	)

(call my-tree print-value-mul 15)