// Included inside tune_document_checks.cpp's private namespace, after last_key.
// Literal JSON fixture families retain their original assertion and invocation order.

#include "scripts/acceptance/debug_console/tune_colour_document_splices.h"
#include "scripts/acceptance/debug_console/tune_colour_document_feedback.h"
#include "scripts/acceptance/debug_console/tune_colour_document_opening.h"
#include "scripts/acceptance/debug_console/tune_colour_document_hud_figure.h"

	void check_colour_document()
	{
		check_colour_document_splices();
		check_colour_document_selectors_feedback();
		check_colour_document_opening_art();
		check_colour_document_hud();
		check_colour_document_opening_details();
		check_colour_document_figure_identity();
	}
