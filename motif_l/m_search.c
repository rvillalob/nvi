/*-
 * Copyright (c) 1996
 *	Rob Zimmermann.  All rights reserved.
 * Copyright (c) 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "$Id: m_search.c,v 8.1 1996/12/03 10:31:09 bostic Exp $ (Berkeley) $Date: 1996/12/03 10:31:09 $";
#endif /* not lint */


/* context */
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>


/* types */

typedef struct sds {
    struct sds	*next;
    Widget	shell;
} save_dialog;

static	save_dialog	*dialogs = NULL;

typedef struct	{
    String	name;
    Boolean	is_default;
    void	(*cb)();
} ButtonData;

typedef struct	{
    String	name;
    Boolean	*value;
} ToggleData;

typedef enum {
    ExprStandard,
    ExprExtended,
    ExprNone
} ExpressionKind;

static	String	expr_image[] = {
    "Standard",
    "Extended",
    "None"
};


/* globals and constants */

static	String	ExpressionWidget = "option",
		PatternWidget	 = "text";

static	Boolean ignore_case,
		wrap_search,
		incremental_search;

static	ToggleData	toggle_data[] = {
    { "Ignore Case",		&ignore_case		},
    { "Wrap End Of File", 	&wrap_search		},
    { "Incremental", 		&incremental_search	}
};

void		next_func(),
		prev_func(),
		done_func();

static	ButtonData button_data[] = {
    { "Next",		True,	next_func	},
    { "Previous", 	False,	prev_func	},
    { "Cancel", 	False,	done_func	}
};

ExpressionKind	expr_kind;

String		pattern = NULL;


/* Xt utilities */

#if defined(__STDC__)
Widget	get_child_widget( Widget parent, String name )
#else
Widget	get_child_widget( parent, name )
Widget	parent;
String	name;
#endif
{
    char buffer[1024];

    strcpy( buffer, "*" );
    strcat( buffer, name );
    return XtNameToWidget( parent, buffer );
}


#if defined(__STDC__)
String	get_widget_string( Widget w, String resource )
#else
String	get_widget_string( w, resource )
Widget	w;
String	resource;
#endif
{
    XmString	xmstr;
    String	str;

    XtVaGetValues( w, resource, &xmstr, 0 );
    XmStringGetLtoR( xmstr, XmSTRING_DEFAULT_CHARSET, &str );
    XmStringFree( xmstr );

    return str;
}


/* sync the global state */

#if defined(__STDC__)
static	void	get_state( Widget w )
#else
static	void	get_state( w )
	Widget	w;
#endif
{
    int		i;
    String	str;
    Widget	shell = w;

    /* get all the data from the root of the widget tree */
    while ( ! XtIsShell(shell) ) shell = XtParent(shell);

    /* which regular expression kind? */
    if (( w = get_child_widget( shell, ExpressionWidget )) != NULL ) {
	w   = XmOptionButtonGadget( w );
	str = get_widget_string( w, XmNlabelString );
	for (i=0; i<XtNumber(expr_image); i++) {
	    if ( strcmp( str, expr_image[i] ) == 0 ) {
		expr_kind = (ExpressionKind) i;
		break;
	    }
	}
	XtFree( str );
    }

    /* which flags? */
    for (i=0; i<XtNumber(toggle_data); i++) {
	if (( w = get_child_widget( shell, toggle_data[i].name )) != NULL ) {
	    XtVaGetValues( w, XmNset, toggle_data[i].value, 0 );
	}
    }

    /* what's the pattern? */
    if (( w = get_child_widget( shell, PatternWidget )) != NULL ) {
	if ( pattern != NULL ) XtFree( pattern );
	pattern = XmTextFieldGetString( w );
    }
}


/* Translate the user's actions into nvi commands */

#if defined(__STDC__)
static	void	send_command( Widget w )
#else
static	void	send_command( w )
	Widget	w;
#endif
{
    String	safe_pattern = (pattern) ? pattern : "";
    int		i;

#if defined(SelfTest)
    printf( "%s [\n", (w == NULL) ? "Find Next" : XtName(w) );
    printf( "\tPattern\t\t%s\n", safe_pattern );
    printf( "\tExpression Kind\t%s\n", expr_image[(int)expr_kind] );
    for( i=0; i<XtNumber(toggle_data); i++ )
	printf( "\t%s\t%d\n", toggle_data[i].name, *toggle_data[i].value );
    printf( "]\n" );
#else
    {
    char buffer[1024];

    strcpy( buffer, "/" );
    strcat( buffer, pattern );
    send_command_string( buffer );
    }
#endif
}


#if defined(__STDC__)
static	void	next_func( Widget w )
#else
static	void	next_func( w )
	Widget	w;
#endif
{
    /* get current data from the root of the widget tree? */
    if ( w != NULL ) get_state( w );

    /* format it */
    send_command( w );
}


#if defined(__STDC__)
static	void	prev_func( Widget w )
#else
static	void	prev_func( w )
	Widget	w;
#endif
{
    /* get current data from the root of the widget tree */
    get_state( w );

    /* format it */
    send_command( w );
}


#if defined(__STDC__)
static	void	done_func( Widget w )
#else
static	void	done_func( w )
	Widget	w;
#endif
{
    save_dialog	*ptr;

#if defined(SelfTest)
    puts( XtName(w) );
#endif

    while ( ! XtIsShell(w) ) w = XtParent(w);
    XtPopdown( w );

    /* save it for later */
    ptr		= (save_dialog *) malloc( sizeof(save_dialog) );
    ptr->next	= dialogs;
    ptr->shell	= w;
    dialogs	= ptr;
}


/* create a set of push buttons */

#define	SpacingRatio	4	/* 3:1 button to spaces */

#if defined(__STDC__)
static	Widget	create_push_buttons( Widget parent,
				     ButtonData *data,
				     int count
				    )
#else
static	Widget	create_push_buttons( parent, data, count )
    Widget	parent;
    ButtonData	*data;
    int		count;
#endif
{
    Widget	w, form;
    int		pos = 1, base;

    base = SpacingRatio*count + 1;
    form = XtVaCreateManagedWidget( "buttons", 
				    xmFormWidgetClass,
				    parent,
				    XmNleftAttachment,	XmATTACH_FORM,
				    XmNrightAttachment,	XmATTACH_FORM,
				    XmNfractionBase,	base,
				    XmNshadowType,	XmSHADOW_ETCHED_IN,
				    XmNshadowThickness,	2,
				    XmNverticalSpacing,	4,
				    0
				    );

    while ( count-- > 0 ) {
	w = XtVaCreateManagedWidget(data->name,
				    xmPushButtonGadgetClass,
				    form,
				    XmNtopAttachment,	XmATTACH_FORM,
				    XmNbottomAttachment,XmATTACH_FORM,
				    XmNleftAttachment,	XmATTACH_POSITION,
				    XmNleftPosition,	pos,
				    XmNshowAsDefault,	data->is_default,
				    XmNdefaultButtonShadowThickness,	data->is_default,
				    XmNrightAttachment,	XmATTACH_POSITION,
				    XmNrightPosition,	pos+SpacingRatio-1,
				    0
				    );
	if ( data->is_default )
	    XtVaSetValues( form, XmNdefaultButton, w, 0 );
	XtAddCallback( w, XmNactivateCallback, data->cb, 0 );
	pos += SpacingRatio;
	data++;
    }

    return form;
}


/* create a set of check boxes */

#if defined(__STDC__)
static	Widget	create_check_boxes( Widget parent,
				    ToggleData *toggles,
				    int count
				    )
#else
static	Widget	create_check_boxes( parent, toggles, count )
	Widget	parent;
	ToggleData *toggles;
	int	count;
#endif
{
    Widget	form;
    int		pos = 1, base;

    base = SpacingRatio*count +1;
    form = XtVaCreateManagedWidget( "toggles", 
				    xmFormWidgetClass,
				    parent,
				    XmNleftAttachment,	XmATTACH_FORM,
				    XmNrightAttachment,	XmATTACH_FORM,
				    XmNfractionBase,	base,
				    XmNverticalSpacing,	4,
				    0
				    );

    while ( count-- > 0 ) {
	XtVaCreateManagedWidget(toggles->name,
				xmToggleButtonWidgetClass,
				form,
				XmNtopAttachment,	XmATTACH_FORM,
				XmNbottomAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_POSITION,
				XmNleftPosition,	pos,
				XmNrightAttachment,	XmATTACH_POSITION,
				XmNrightPosition,	pos+SpacingRatio-1,
				0
				);
	pos += SpacingRatio;
	++toggles;
    }

    return form;
}


/* Routines to handle the text field widget */

/* when the user hits 'CR' in a text widget, fire the default pushbutton */
#if defined(__STDC__)
static	void	text_cr( Widget w, void *ptr, void *ptr2 )
#else
static	void	text_cr( w, ptr, ptr2 )
	Widget	w;
	void	*ptr;
	void	*ptr2;
#endif
{
    next_func( w );
}


/* when the user hits any other character, if we are doing incremental
 * search, send the updated string to nvi
 */
#if defined(__STDC__)
static	void	value_changed( Widget w, void *ptr, void *ptr2 )
#else
static	void	value_changed( w, ptr, ptr2 )
	Widget	w;
	void	*ptr;
	void	*ptr2;
#endif
{
    /* get all the data from the root of the widget tree */
    get_state( w );

    /* send it along? */
    if ( incremental_search ) send_command( w );
}


/* Draw and display a dialog the describes nvi search capability */

#if defined(__STDC__)
Widget	create_search_dialog( Widget parent, String title )
#else
Widget	create_search_dialog( parent, title )
Widget	parent;
String	title;
#endif
{
    Widget	box, form, label, text, checks, options, buttons, form2;
    XmString	opt_str, reg_str, xtd_str, non_str;
    save_dialog	*ptr;

    /* use an existing one? */
    if ( dialogs != NULL ) {
	box = dialogs->shell;
	ptr = dialogs->next;
	free( dialogs );
	dialogs = ptr;
	return box;
    }

    box = XtVaCreatePopupShell( title,
				xmDialogShellWidgetClass,
				parent,
				XmNtitle,		title,
				XmNallowShellResize,	False,
				0
				);

    form = XtVaCreateWidget( "form", 
				xmFormWidgetClass,
				box,
				XmNverticalSpacing,	4,
				XmNhorizontalSpacing,	4,
				0
				);

    form2 = XtVaCreateManagedWidget( "form", 
				xmFormWidgetClass,
				form,
				XmNtopAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNrightAttachment,	XmATTACH_FORM,
				0
				);

    label = XtVaCreateManagedWidget( "Pattern:", 
				    xmLabelWidgetClass,
				    form2,
				    XmNalignment,	XmALIGNMENT_END,
				    XmNtopAttachment,	XmATTACH_FORM,
				    XmNbottomAttachment,XmATTACH_FORM,
				    XmNrightAttachment,	XmATTACH_POSITION,
				    XmNrightPosition,	25,
				    0
				    );

    text = XtVaCreateManagedWidget( PatternWidget, 
				    xmTextFieldWidgetClass,
				    form2,
				    XmNtopAttachment,	XmATTACH_FORM,
				    XmNbottomAttachment,XmATTACH_FORM,
				    XmNleftAttachment,	XmATTACH_POSITION,
				    XmNleftPosition,	25,
				    XmNrightAttachment,	XmATTACH_FORM,
				    0
				    );
    XtAddCallback( text, XmNvalueChangedCallback, value_changed, 0 );
    XtAddCallback( text, XmNactivateCallback, text_cr, 0 );

    opt_str = XmStringCreateSimple( "Regular Expression Type" );
    reg_str = XmStringCreateSimple( expr_image[(int)ExprStandard] );
    xtd_str = XmStringCreateSimple( expr_image[(int)ExprExtended] );
    non_str = XmStringCreateSimple( expr_image[(int)ExprNone] );
    options = XmVaCreateSimpleOptionMenu( form,
				    ExpressionWidget,
				    opt_str,
				    0,
				    0,
				    NULL,
				    XmVaPUSHBUTTON, reg_str, 0, 0, 0,
				    XmVaPUSHBUTTON, xtd_str, 0, 0, 0,
				    XmVaPUSHBUTTON, non_str, 0, 0, 0,
				    XmNtopAttachment,	XmATTACH_WIDGET,
				    XmNtopWidget,	form2,
				    XmNleftAttachment,	XmATTACH_POSITION,
				    XmNleftPosition,	25,
				    XmNrightAttachment,	XmATTACH_FORM,
				    0
				    );
    XmStringFree( opt_str );
    XmStringFree( reg_str );
    XmStringFree( xtd_str );
    XmStringFree( non_str );
    XtManageChild( options );

    buttons = create_push_buttons( form, button_data, XtNumber(button_data) );
    XtVaSetValues( buttons,
		   XmNbottomAttachment,	XmATTACH_FORM,
		   0
		   );

    checks = create_check_boxes( form, toggle_data, XtNumber(toggle_data) );
    XtVaSetValues( checks,
		   XmNtopAttachment,	XmATTACH_WIDGET,
		   XmNtopWidget,	options,
		   XmNbottomAttachment,	XmATTACH_WIDGET,
		   XmNbottomWidget,	buttons,
		   0
		   );

    XtManageChild( form );
    return box;
}


/* Module interface to the outside world
 *
 *	xip_show_search_dialog( parent, title )
 *	pops up a search dialog
 *
 *	xip_next_search()
 *	simulates a 'next' assuming that a search has been done
 */

#if defined(__STDC__)
void 	xip_show_search_dialog( Widget parent, String title )
#else
void	xip_show_search_dialog( parent, data, cbs )
Widget	parent;
String	title;
#endif
{
    Widget 	db = create_search_dialog( parent, title );
    Dimension	height;

    /* we can handle getting taller and wider or narrower, but not shorter */
    XtVaGetValues( db, XmNheight, &height, 0 );
    XtVaSetValues( db, XmNminHeight, height, 0 );

    /* post the dialog */
    XtPopup( db, XtGrabNone );

    /* request initial focus to the text widget */
    XmProcessTraversal( get_child_widget( db, PatternWidget ),
			XmTRAVERSE_CURRENT
			);
}


void	xip_next_search()
{
    next_func( NULL );
}


#if defined(SelfTest)

#if XtSpecificationRelease == 4
#define	ArgcType	Cardinal *
#else
#define	ArgcType	int *
#endif

#if defined(__STDC__)
static void show_search( Widget w, XtPointer data, XtPointer cbs )
#else
static void show_search( w, data, cbs )
Widget w;
XtPointer	data;
XtPointer	cbs;
#endif
{
    xip_show_search_dialog( data, "Search" );
}

main( int argc, char *argv[] )
{
    XtAppContext	ctx;
    Widget		top_level, rc, button;
    extern		exit();

    /* create a top-level shell for the window manager */
    top_level = XtVaAppInitialize( &ctx,
				   argv[0],
				   NULL, 0,	/* options */
				   (ArgcType) &argc,
				   argv,	/* might get modified */
				   NULL,
				   NULL
				   );

    rc = XtVaCreateManagedWidget( "rc",
				  xmRowColumnWidgetClass,
				  top_level,
				  0
				  );

    button = XtVaCreateManagedWidget( "Pop up search dialog",
				      xmPushButtonGadgetClass,
				      rc,
				      0
				      );
    XtAddCallback( button, XmNactivateCallback, show_search, rc );

    button = XtVaCreateManagedWidget( "Quit",
				      xmPushButtonGadgetClass,
				      rc,
				      0
				      );
    XtAddCallback( button, XmNactivateCallback, exit, 0 );

    XtRealizeWidget(top_level);
    XtAppMainLoop(ctx);
}

#endif
