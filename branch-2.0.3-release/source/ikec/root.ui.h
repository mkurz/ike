
/*
 * Copyright (c) 2007
 *      Shrew Soft Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Redistributions in any form must be accompanied by information on
 *    how to obtain complete source code for the software and any
 *    accompanying software that uses the software.  The source code
 *    must either be included in the distribution or be available for no
 *    more than the cost of distribution plus a nominal fee, and must be
 *    freely redistributable under reasonable conditions.  For an
 *    executable file, complete source code means the source code for all
 *    modules it contains.  It does not include source code for modules or
 *    files that typically accompany the major components of the operating
 *    system on which the executable file runs.
 *
 * THIS SOFTWARE IS PROVIDED BY SHREW SOFT INC ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 * NON-INFRINGEMENT, ARE DISCLAIMED.  IN NO EVENT SHALL SHREW SOFT INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * AUTHOR : Matthew Grooms
 *          mgrooms@shrew.net
 *
 */

#include "ikec.h"

void root::SiteConnect()
{
	if( ikec.active )
		ikec.cancel = true;
	else
	{
		// if enabled, verify that a valid
		// username and password was supplied

		if( !groupBoxCredentials->isHidden() )
		{
			if( !lineEditUsername->text().length() ||
			    !lineEditPassword->text().length() )
			{
				textBrowserStatus->append( 
					"please enter a valid username and password\n" );
				return;
			}
		}

		// store username and password

		ikec.username = lineEditUsername->text().ascii();
		ikec.password = lineEditPassword->text().ascii();

		// start our thread

		ikec.start();
	}
}


void root::SiteDisconnect()
{
	if( ikec.active )
		ikec.cancel = true;
	else
		close();
}

void root::customEvent( QCustomEvent * e )
{
	if( e->type() == EVENT_RUNNING )
	{
		RunningEvent * event = ( RunningEvent * ) e;

		if( event->running )
		{
			textLabelStatusValue->setText( "Connecting" );

			lineEditUsername->setEnabled( false );
			lineEditPassword->setEnabled( false );

			pushButtonConnect->setEnabled( false );
			pushButtonConnect->setText( "Connect" );

			pushButtonExit->setEnabled( true );
			pushButtonExit->setText( "Cancel" );

			textLabelRemoteValue->setText( event->host );
		}
		else
		{
			textLabelStatusValue->setText( "Disconnected" );

			lineEditUsername->setEnabled( true );
			lineEditPassword->setEnabled( true );

			pushButtonConnect->setEnabled( true );
			pushButtonConnect->setText( "Connect" );

			pushButtonExit->setEnabled( true );
			pushButtonExit->setText( "Exit" );

			textLabelRemoteValue->setText( event->host );
		}
	}

	if( e->type() == EVENT_ENABLE )
	{
		EnableEvent * event = ( EnableEvent * ) e;

		if( event->enabled )
			textBrowserStatus->append( "bringing up tunnel ...\n" );
		else
			textBrowserStatus->append( "bringing down tunnel ...\n" );
	}

	if( e->type() == EVENT_STATUS )
	{
		StatusEvent * event = ( StatusEvent * ) e;

		switch( event->status )
		{
			case STATUS_WARN:

				textBrowserStatus->setColor( QColor( 192, 128, 0 ) );

				break;

			case STATUS_FAIL:

				textBrowserStatus->setColor( QColor( 128, 0, 0 ) );

				break;

			default:

				textBrowserStatus->setColor( QColor( 0, 0, 0 ) );
		}

		switch( event->status )
		{
			case STATUS_BANNER:
			{
				banner b( this );
				b.textBrowserMOTD->setText( event->text );
				b.exec();

				break;
			}

			case STATUS_ENABLED:

				textLabelStatusValue->setText( "Connected" );

				pushButtonConnect->setEnabled( true );
				pushButtonConnect->setText( "Disconnect" );

				pushButtonExit->setEnabled( false );
				pushButtonExit->setText( "Cancel" );

				textBrowserStatus->append( event->text );

				break;

			case STATUS_DISABLED:
			case STATUS_INFO:
			case STATUS_WARN:
			case STATUS_FAIL:

				textBrowserStatus->append( event->text );

				break;

			default:

				textBrowserStatus->append( "!!! unknown status message !!!\n" );
		}
	}

	if( e->type() == EVENT_STATS )
	{
		StatsEvent * event = ( StatsEvent * ) e;

		QString n;

		n.setNum( event->stats.sa_good );
		textLabelEstablishedValue->setText( n );

		n.setNum( event->stats.sa_dead );
		textLabelExpiredValue->setText( n );

		n.setNum( event->stats.sa_fail );
		textLabelFailedValue->setText( n );

		if( event->stats.natt )
			textLabelTransportValue->setText( "NAT-T / IKE | ESP" );
		else
			textLabelTransportValue->setText( "IKE | ESP" );

		if( event->stats.frag )
			textLabelFragValue->setText( "Enabled" );
		else
			textLabelFragValue->setText( "Disabled" );

		if( event->stats.dpd )
			textLabelDPDValue->setText( "Enabled" );
		else
			textLabelDPDValue->setText( "Disabled" );
	}

	if( e->type() == EVENT_FILEPASS )
	{
		FilePassEvent * event = ( FilePassEvent * ) e;

		filepass fp;
		QFileInfo pathInfo( event->PassData->filepath );
		fp.setCaption( "Password for " + pathInfo.fileName());
		event->PassData->result = fp.exec();
		event->PassData->password = fp.lineEditPassword->text();
	}
}
