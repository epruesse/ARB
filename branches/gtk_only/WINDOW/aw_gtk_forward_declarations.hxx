/* 
 * File:   AW_gtk_forward_declarations.hxx
 * Author: aboeckma
 *
 * Contains forward declarations for all gui types
 * Created on October 16, 2012, 10:04 AM
 */


#pragma once
struct _GtkWidget; 
struct _GtkMenuBar;
struct _GtkTreeView;
struct _GtkListStore;
struct _GtkTreeSelection;
union _GdkEvent;
struct _GdkEventConfigure;
struct _GdkEventExpose;
struct _GdkEventKey;
struct _GdkEventMotion;

typedef struct _GtkWidget         GtkWidget;
typedef struct _GtkMenuBar        GtkMenuBar;
typedef struct _GtkTreeView       GtkTreeView;
typedef struct _GtkListStore      GtkListStore;
typedef struct _GtkTreeSelection  GtkTreeSelection;
typedef union  _GdkEvent          GdkEvent;
typedef struct _GdkEventConfigure GdkEventConfigure;
typedef struct _GdkEventExpose    GdkEventExpose;
typedef struct _GdkEventKey       GdkEventKey;
typedef struct _GdkEventMotion    GdkEventMotion;
