/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "CMouseDispatcher.h"
#include "macutil.h"


// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CMouseDispatcher::CMouseDispatcher()
	:	LAttachment(msg_Event, true)
{
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CMouseDispatcher::ExecuteSelf(
	MessageT			inMessage,
	void*				ioParam)
{
	Assert_(inMessage == msg_Event);
	const EventRecord* theEvent = (const EventRecord*)ioParam;
	
	// High level events dont have the mouse position in the where field
	// of the event record.
	if (theEvent->what == kHighLevelEvent)
		return;

	// Don't process mouse position for update events (otherwise, tooltip
	// panes that are drawn under the mouse position get deleted before
	// the update event even gets processed - jrm 98/05/05.
	if (theEvent->what == updateEvt)
		return;

	LPane* theCurrentPane = nil;
	LPane* theLastPane = LPane::GetLastPaneMoused();

	Point thePortMouse;	
	WindowPtr theMacWindow;
	Int16 thePart = ::FindWindow(theEvent->where, &theMacWindow);
	LWindow	*theWindow = LWindow::FetchWindowObject(theMacWindow);

	if (theWindow != nil)
	{
		thePortMouse = theEvent->where;
		theWindow->GlobalToPortPoint(thePortMouse);

		// Note: it is the responsibility of the pane
		// to guard with IsEnabled(), IsActive(), etc.
		// Some panes need to execute MouseEnter, etc
		// even in inactive windows
		
		theCurrentPane = FindPaneHitBy(theEvent->where);
	}


	// It is okay to assume that the mouse point was converted to port coordinates
	// in the conditional above.  If it was not, it's because the current pane will
	// be nil, and the only thing that will be called later is MouseLeave() which
	// dosen't need it.
	SMouseTrackParms theMouseParms;
	theMouseParms.portMouse = thePortMouse;
	theMouseParms.macEvent = *theEvent;
	theMouseParms.paneOfAttachment = nil; // will be changed by an attachment to show it's there.

	// At this point, even if theCurrentPane and theLastPane are different, it
	// might be because theCurrentPane is a tooltip pane, put there by the
	// attachment.  This is no reason for doing "mouse left" processing.
	// Use theLastPane instead.
	if (theCurrentPane != theLastPane
	&& theLastPane  && theLastPane->Contains(thePortMouse.h, thePortMouse.v))
		{
		if (theLastPane->ExecuteAttachments(msg_MouseWithin, &theMouseParms)
		&& theMouseParms.paneOfAttachment == theCurrentPane)
			{
			theLastPane->MouseWithin(thePortMouse, *theEvent);
			if (theCurrentPane != nil) // see explanation below.
				theCurrentPane = FindPaneHitBy(theEvent->where);
			}
		}
	if ((!theCurrentPane || theMouseParms.paneOfAttachment != theCurrentPane)
	&& theCurrentPane != theLastPane)
		{
		if (theLastPane != nil)
			{
			if (theLastPane->ExecuteAttachments(msg_MouseLeft, &theMouseParms))
				{
				theLastPane->MouseLeave();
				// If theLastPane is a CPatternButton with a tooltip attachment,
				// the ExecuteAttachments() method above deletes the tooltip pane.
				// This is normal behaviour. However, if we are unlucky enough,
				// theCurrentPane can point to the tooltip pane which has just
				// been deleted. So it is important to re-initialize theCurrentPane
				// at this point:
				if (theCurrentPane != nil)
					theCurrentPane = FindPaneHitBy(theEvent->where);
				}

			}
		
		if (theCurrentPane != nil)
			{
			if (theCurrentPane->ExecuteAttachments(msg_MouseEntered, &theMouseParms))
				theCurrentPane->MouseEnter(thePortMouse, *theEvent);
			}
		
		LPane::SetLastPaneMoused(theCurrentPane);
		}
	else if (theCurrentPane != nil)
		{
		if (theCurrentPane->ExecuteAttachments(msg_MouseWithin, &theMouseParms))
			theCurrentPane->MouseWithin(thePortMouse, *theEvent);
		}
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CMouseTrackAttachment::CMouseTrackAttachment()
	:	LAttachment(msg_AnyMessage)
	,	mOwningPane(nil)
	,	mPaneMustBeActive(true)
	,	mWindowMustBeActive(true)
	,	mMustBeEnabled(true)
{
	SetExecuteHost(true);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CMouseTrackAttachment::CMouseTrackAttachment(LStream* inStream)
	:	LAttachment(inStream)
	,	mOwningPane(nil)
	,	mPaneMustBeActive(true)
	,	mWindowMustBeActive(true)
	,	mMustBeEnabled(true)
{
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

Boolean CMouseTrackAttachment::EnsureOwningPane()
{
	if (!mOwningPane)
	{
		mOwningPane = dynamic_cast<LPane*>(GetOwnerHost());
		Assert_(mOwningPane); 	// you didn't attach to an LPane* derivative
	}
	if (!mOwningPane)
		return false;
	if (mMustBeEnabled && !mOwningPane->IsEnabled())
		return false;
	if (mPaneMustBeActive && !mOwningPane->IsActive())
		return false;
	if (mWindowMustBeActive)
	{
		LWindow* win
			= dynamic_cast<LWindow*>(
				LWindow::FetchWindowObject(mOwningPane->GetMacPort()));
		if (!win || !win->IsActive())
			return false;
	}
	return true;
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CMouseTrackAttachment::ExecuteSelf(
	MessageT			inMessage,
	void*				ioParam)
{
	SMouseTrackParms* theTrackParms = (SMouseTrackParms*)ioParam;

	if (!EnsureOwningPane())
		return;
		
	switch (inMessage)
		{
		case msg_MouseEntered:
			Assert_(theTrackParms != nil);
			if ( theTrackParms )
				MouseEnter(theTrackParms->portMouse, theTrackParms->macEvent);
			break;
			
		case msg_MouseWithin:
			Assert_(theTrackParms != nil);
			if ( theTrackParms )
				MouseWithin(theTrackParms->portMouse, theTrackParms->macEvent);
			break;
			
		case msg_MouseLeft:
			MouseLeave();
			break;	
		}
};

