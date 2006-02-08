/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "html_objectimpl.h"

#include "EventNames.h"
#include "Frame.h"
#include "HTMLFormElementImpl.h"
#include "csshelper.h"
#include "cssproperties.h"
#include "cssstyleselector.h"
#include "cssvalues.h"
#include "dom2_eventsimpl.h"
#include "PlatformString.h"
#include "dom_textimpl.h"
#include "html_documentimpl.h"
#include "html_imageimpl.h"
#include "render_applet.h"
#include "render_frames.h"
#include "render_image.h"
#include <java/kjavaappletwidget.h>
#include <QString.h>
#include "htmlnames.h"

namespace WebCore {

using namespace EventNames;
using namespace HTMLNames;

// -------------------------------------------------------------------------

HTMLAppletElementImpl::HTMLAppletElementImpl(DocumentImpl *doc)
  : HTMLElementImpl(appletTag, doc)
{
    appletInstance = 0;
    m_allParamsAvailable = false;
}

HTMLAppletElementImpl::~HTMLAppletElementImpl()
{
    // appletInstance should have been cleaned up in detach().
    assert(!appletInstance);
}

bool HTMLAppletElementImpl::checkDTD(const NodeImpl* newChild)
{
    return newChild->hasTagName(paramTag) || HTMLElementImpl::checkDTD(newChild);
}

bool HTMLAppletElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr) {
            result = eUniversal;
            return false;
    }
    
    if (attrName == alignAttr) {
        result = eReplaced; // Share with <img> since the alignment behavior is the same.
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLAppletElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == altAttr ||
        attr->name() == archiveAttr ||
        attr->name() == codeAttr ||
        attr->name() == codebaseAttr ||
        attr->name() == mayscriptAttr ||
        attr->name() == objectAttr) {
        // Do nothing.
    } else if (attr->name() == widthAttr) {
        addCSSLength(attr, CSS_PROP_WIDTH, attr->value());
    } else if (attr->name() == heightAttr) {
        addCSSLength(attr, CSS_PROP_HEIGHT, attr->value());
    } else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_BOTTOM, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_RIGHT, attr->value());
    } else if (attr->name() == alignAttr) {
        addHTMLAlignment(attr);
    } else if (attr->name() == nameAttr) {
        DOMString newNameAttr = attr->value();
        if (inDocument() && getDocument()->isHTMLDocument()) {
            HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
            document->removeNamedItem(oldNameAttr);
            document->addNamedItem(newNameAttr);
        }
        oldNameAttr = newNameAttr;
    } else if (attr->name() == idAttr) {
        DOMString newIdAttr = attr->value();
        if (inDocument() && getDocument()->isHTMLDocument()) {
            HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
            document->removeDocExtraNamedItem(oldIdAttr);
            document->addDocExtraNamedItem(newIdAttr);
        }
        oldIdAttr = newIdAttr;
        // also call superclass
        HTMLElementImpl::parseMappedAttribute(attr);
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

void HTMLAppletElementImpl::insertedIntoDocument()
{
    if (getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->addNamedItem(oldNameAttr);
        document->addDocExtraNamedItem(oldIdAttr);
    }

    HTMLElementImpl::insertedIntoDocument();
}

void HTMLAppletElementImpl::removedFromDocument()
{
    if (getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->removeNamedItem(oldNameAttr);
        document->removeDocExtraNamedItem(oldIdAttr);
    }

    HTMLElementImpl::removedFromDocument();
}

bool HTMLAppletElementImpl::rendererIsNeeded(RenderStyle *style)
{
    return !getAttribute(codeAttr).isNull();
}

RenderObject *HTMLAppletElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
#ifndef Q_WS_QWS // FIXME(E)? I don't think this is possible with Qt Embedded...
    Frame *frame = getDocument()->frame();

    if( frame && frame->javaEnabled() )
    {
        HashMap<DOMString, DOMString> args;

        args.set("code", getAttribute(codeAttr));
        const AtomicString& codeBase = getAttribute(codebaseAttr);
        if(!codeBase.isNull())
            args.set("codeBase", codeBase);
        const AtomicString& name = getAttribute(getDocument()->htmlMode() != DocumentImpl::XHtml ? nameAttr : idAttr);
        if (!name.isNull())
            args.set("name", name);
        const AtomicString& archive = getAttribute(archiveAttr);
        if (!archive.isNull())
            args.set("archive", archive);

        args.set("baseURL", getDocument()->baseURL());

        const AtomicString& mayScript = getAttribute(mayscriptAttr);
        if (!mayScript.isNull())
            args.set("mayScript", mayScript);

        // Other arguments (from <PARAM> tags) are added later.
        
        return new (getDocument()->renderArena()) RenderApplet(this, args);
    }

    // ### remove me. we should never show an empty applet, instead
    // render the alternative content given by the webpage
    return new (getDocument()->renderArena()) RenderEmptyApplet(this);
#else
    return 0;
#endif
}

KJS::Bindings::Instance *HTMLAppletElementImpl::getAppletInstance() const
{
    Frame *frame = getDocument()->frame();
    if (!frame || !frame->javaEnabled())
        return 0;

    if (appletInstance)
        return appletInstance;
    
    RenderApplet *r = static_cast<RenderApplet*>(renderer());
    if (r) {
        r->createWidgetIfNecessary();
        if (r->widget())
            // Call into the frame (and over the bridge) to pull the Bindings::Instance
            // from the guts of the plugin.
            appletInstance = frame->getAppletInstanceForWidget(r->widget());
    }
    return appletInstance;
}

void HTMLAppletElementImpl::closeRenderer()
{
    // The parser just reached </applet>, so all the params are available now.
    m_allParamsAvailable = true;
    if (renderer())
        renderer()->setNeedsLayout(true); // This will cause it to create its widget & the Java applet
    HTMLElementImpl::closeRenderer();
}

void HTMLAppletElementImpl::detach()
{
    // Delete appletInstance, because it references the widget owned by the renderer we're about to destroy.
    if (appletInstance) {
        delete appletInstance;
        appletInstance = 0;
    }

    HTMLElementImpl::detach();
}

bool HTMLAppletElementImpl::allParamsAvailable()
{
    return m_allParamsAvailable;
}

DOMString HTMLAppletElementImpl::align() const
{
    return getAttribute(alignAttr);
}

void HTMLAppletElementImpl::setAlign(const DOMString &value)
{
    setAttribute(alignAttr, value);
}

DOMString HTMLAppletElementImpl::alt() const
{
    return getAttribute(altAttr);
}

void HTMLAppletElementImpl::setAlt(const DOMString &value)
{
    setAttribute(altAttr, value);
}

DOMString HTMLAppletElementImpl::archive() const
{
    return getAttribute(archiveAttr);
}

void HTMLAppletElementImpl::setArchive(const DOMString &value)
{
    setAttribute(archiveAttr, value);
}

DOMString HTMLAppletElementImpl::code() const
{
    return getAttribute(codeAttr);
}

void HTMLAppletElementImpl::setCode(const DOMString &value)
{
    setAttribute(codeAttr, value);
}

DOMString HTMLAppletElementImpl::codeBase() const
{
    return getAttribute(codebaseAttr);
}

void HTMLAppletElementImpl::setCodeBase(const DOMString &value)
{
    setAttribute(codebaseAttr, value);
}

DOMString HTMLAppletElementImpl::height() const
{
    return getAttribute(heightAttr);
}

void HTMLAppletElementImpl::setHeight(const DOMString &value)
{
    setAttribute(heightAttr, value);
}

DOMString HTMLAppletElementImpl::hspace() const
{
    return getAttribute(hspaceAttr);
}

void HTMLAppletElementImpl::setHspace(const DOMString &value)
{
    setAttribute(hspaceAttr, value);
}

DOMString HTMLAppletElementImpl::name() const
{
    return getAttribute(nameAttr);
}

void HTMLAppletElementImpl::setName(const DOMString &value)
{
    setAttribute(nameAttr, value);
}

DOMString HTMLAppletElementImpl::object() const
{
    return getAttribute(objectAttr);
}

void HTMLAppletElementImpl::setObject(const DOMString &value)
{
    setAttribute(objectAttr, value);
}

DOMString HTMLAppletElementImpl::vspace() const
{
    return getAttribute(vspaceAttr);
}

void HTMLAppletElementImpl::setVspace(const DOMString &value)
{
    setAttribute(vspaceAttr, value);
}

DOMString HTMLAppletElementImpl::width() const
{
    return getAttribute(widthAttr);
}

void HTMLAppletElementImpl::setWidth(const DOMString &value)
{
    setAttribute(widthAttr, value);
}

// -------------------------------------------------------------------------

HTMLEmbedElementImpl::HTMLEmbedElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(embedTag, doc), embedInstance(0)
{}

HTMLEmbedElementImpl::~HTMLEmbedElementImpl()
{
    // embedInstance should have been cleaned up in detach().
    assert(!embedInstance);
}

bool HTMLEmbedElementImpl::checkDTD(const NodeImpl* newChild)
{
    return newChild->hasTagName(paramTag) || HTMLElementImpl::checkDTD(newChild);
}

KJS::Bindings::Instance *HTMLEmbedElementImpl::getEmbedInstance() const
{
    Frame *frame = getDocument()->frame();
    if (!frame)
        return 0;

    if (embedInstance)
        return embedInstance;
    
    RenderObject *r = renderer();
    if (!r) {
        NodeImpl *p = parentNode();
        if (p && p->hasTagName(objectTag))
            r = p->renderer();
    }

    if (r && r->isWidget()){
        if (QWidget *widget = static_cast<RenderWidget *>(r)->widget()) {
            // Call into the frame (and over the bridge) to pull the Bindings::Instance
            // from the guts of the Java VM.
            embedInstance = frame->getEmbedInstanceForWidget(widget);
            // Applet may specified with <embed> tag.
            if (!embedInstance)
                embedInstance = frame->getAppletInstanceForWidget(widget);
        }
    }
    return embedInstance;
}

bool HTMLEmbedElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == borderAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr ||
        attrName == valignAttr ||
        attrName == hiddenAttr) {
        result = eUniversal;
        return false;
    }
        
    if (attrName == alignAttr) {
        result = eReplaced; // Share with <img> since the alignment behavior is the same.
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLEmbedElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    QString val = attr->value().qstring();
  
    int pos;
    if (attr->name() == typeAttr) {
        serviceType = val.lower();
        pos = serviceType.find( ";" );
        if ( pos!=-1 )
            serviceType = serviceType.left( pos );
    } else if (attr->name() == codeAttr ||
               attr->name() == srcAttr) {
         url = khtml::parseURL(attr->value()).qstring();
    } else if (attr->name() == widthAttr) {
        addCSSLength( attr, CSS_PROP_WIDTH, attr->value() );
    } else if (attr->name() == heightAttr) {
        addCSSLength( attr, CSS_PROP_HEIGHT, attr->value());
    } else if (attr->name() == borderAttr) {
        addCSSLength(attr, CSS_PROP_BORDER_WIDTH, attr->value());
        addCSSProperty( attr, CSS_PROP_BORDER_TOP_STYLE, CSS_VAL_SOLID );
        addCSSProperty( attr, CSS_PROP_BORDER_RIGHT_STYLE, CSS_VAL_SOLID );
        addCSSProperty( attr, CSS_PROP_BORDER_BOTTOM_STYLE, CSS_VAL_SOLID );
        addCSSProperty( attr, CSS_PROP_BORDER_LEFT_STYLE, CSS_VAL_SOLID );
    } else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_BOTTOM, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_RIGHT, attr->value());
    } else if (attr->name() == alignAttr) {
        addHTMLAlignment(attr);
    } else if (attr->name() == valignAttr) {
        addCSSProperty(attr, CSS_PROP_VERTICAL_ALIGN, attr->value());
    } else if (attr->name() == pluginpageAttr ||
               attr->name() == pluginspageAttr) {
        pluginPage = val;
    } else if (attr->name() == hiddenAttr) {
        if (val.lower()=="yes" || val.lower()=="true") {
            // FIXME: Not dynamic, but it's not really important that such a rarely-used
            // feature work dynamically.
            addCSSLength( attr, CSS_PROP_WIDTH, "0" );
            addCSSLength( attr, CSS_PROP_HEIGHT, "0" );
        }
    } else if (attr->name() == nameAttr) {
        DOMString newNameAttr = attr->value();
        if (inDocument() && getDocument()->isHTMLDocument()) {
            HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
            document->removeNamedItem(oldNameAttr);
            document->addNamedItem(newNameAttr);
        }
        oldNameAttr = newNameAttr;
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

bool HTMLEmbedElementImpl::rendererIsNeeded(RenderStyle *style)
{
    Frame *frame = getDocument()->frame();
    if (!frame || !frame->pluginsEnabled())
        return false;

    NodeImpl *p = parentNode();
    if (p && p->hasTagName(objectTag)) {
        assert(p->renderer());
        return false;
    }

    return true;
}

RenderObject *HTMLEmbedElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    return new (arena) RenderPartObject(this);
}

void HTMLEmbedElementImpl::attach()
{
    HTMLElementImpl::attach();

    if (renderer())
        static_cast<RenderPartObject*>(renderer())->updateWidget();
}

void HTMLEmbedElementImpl::detach()
{
    // Delete embedInstance, because it references the widget owned by the renderer we're about to destroy.
    if (embedInstance) {
        delete embedInstance;
        embedInstance = 0;
    }

    HTMLElementImpl::detach();
}

void HTMLEmbedElementImpl::insertedIntoDocument()
{
    if (getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->addNamedItem(oldNameAttr);
    }

    HTMLElementImpl::insertedIntoDocument();
}

void HTMLEmbedElementImpl::removedFromDocument()
{
    if (getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->removeNamedItem(oldNameAttr);
    }

    HTMLElementImpl::removedFromDocument();
}

bool HTMLEmbedElementImpl::isURLAttribute(AttributeImpl *attr) const
{
    return attr->name() == srcAttr;
}

// -------------------------------------------------------------------------

HTMLObjectElementImpl::HTMLObjectElementImpl(DocumentImpl *doc) 
: HTMLElementImpl(objectTag, doc), m_imageLoader(0), objectInstance(0)
{
    needWidgetUpdate = false;
    m_useFallbackContent = false;
    m_complete = false;
    m_docNamedItem = true;
}

HTMLObjectElementImpl::~HTMLObjectElementImpl()
{
    // objectInstance should have been cleaned up in detach().
    assert(!objectInstance);
    
    delete m_imageLoader;
}

bool HTMLObjectElementImpl::checkDTD(const NodeImpl* newChild)
{
    return newChild->hasTagName(paramTag) || HTMLElementImpl::checkDTD(newChild);
}

KJS::Bindings::Instance *HTMLObjectElementImpl::getObjectInstance() const
{
    Frame *frame = getDocument()->frame();
    if (!frame)
        return 0;

    if (objectInstance)
        return objectInstance;

    if (RenderObject *r = renderer()) {
        if (r->isWidget()) {
            if (QWidget *widget = static_cast<RenderWidget *>(r)->widget()) {
                // Call into the frame (and over the bridge) to pull the Bindings::Instance
                // from the guts of the plugin.
                objectInstance = frame->getObjectInstanceForWidget(widget);
                // Applet may specified with <object> tag.
                if (!objectInstance)
                    objectInstance = frame->getAppletInstanceForWidget(widget);
            }
        }
    }

    return objectInstance;
}

HTMLFormElementImpl *HTMLObjectElementImpl::form() const
{
    for (NodeImpl *p = parentNode(); p != 0; p = p->parentNode()) {
        if (p->hasTagName(formTag))
            return static_cast<HTMLFormElementImpl *>(p);
    }
    
    return 0;
}

bool HTMLObjectElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr) {
        result = eUniversal;
        return false;
    }
    
    if (attrName == alignAttr) {
        result = eReplaced; // Share with <img> since the alignment behavior is the same.
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLObjectElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    DOMString val = attr->value();
    int pos;
    if (attr->name() == typeAttr) {
        serviceType = val.qstring().lower();
        pos = serviceType.find( ";" );
        if ( pos!=-1 )
          serviceType = serviceType.left( pos );
        if (renderer())
          needWidgetUpdate = true;
        if (!isImageType() && m_imageLoader) {
          delete m_imageLoader;
          m_imageLoader = 0;
        }
    } else if (attr->name() == dataAttr) {
        url = khtml::parseURL(val).qstring();
        if (renderer())
          needWidgetUpdate = true;
        if (renderer() && isImageType()) {
          if (!m_imageLoader)
              m_imageLoader = new HTMLImageLoader(this);
          m_imageLoader->updateFromElement();
        }
    } else if (attr->name() == widthAttr) {
        addCSSLength( attr, CSS_PROP_WIDTH, attr->value());
    } else if (attr->name() == heightAttr) {
        addCSSLength( attr, CSS_PROP_HEIGHT, attr->value());
    } else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_BOTTOM, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(attr, CSS_PROP_MARGIN_RIGHT, attr->value());
    } else if (attr->name() == alignAttr) {
        addHTMLAlignment(attr);
    } else if (attr->name() == classidAttr) {
        classId = val;
        if (renderer())
          needWidgetUpdate = true;
    } else if (attr->name() == onloadAttr) {
        setHTMLEventListener(loadEvent, attr);
    } else if (attr->name() == onunloadAttr) {
        setHTMLEventListener(unloadEvent, attr);
    } else if (attr->name() == nameAttr) {
            DOMString newNameAttr = attr->value();
            if (isDocNamedItem() && inDocument() && getDocument()->isHTMLDocument()) {
                HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
                document->removeNamedItem(oldNameAttr);
                document->addNamedItem(newNameAttr);
            }
            oldNameAttr = newNameAttr;
    } else if (attr->name() == idAttr) {
        DOMString newIdAttr = attr->value();
        if (isDocNamedItem() && inDocument() && getDocument()->isHTMLDocument()) {
            HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
            document->removeDocExtraNamedItem(oldIdAttr);
            document->addDocExtraNamedItem(newIdAttr);
        }
        oldIdAttr = newIdAttr;
        // also call superclass
        HTMLElementImpl::parseMappedAttribute(attr);
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

DocumentImpl* HTMLObjectElementImpl::contentDocument() const
{
    // ###
    return 0;
}

bool HTMLObjectElementImpl::rendererIsNeeded(RenderStyle *style)
{
    if (m_useFallbackContent || isImageType())
        return HTMLElementImpl::rendererIsNeeded(style);

    Frame *frame = getDocument()->frame();
    if (!frame || !frame->pluginsEnabled())
        return false;
    
    return true;
}

RenderObject *HTMLObjectElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    if (m_useFallbackContent)
        return RenderObject::createObject(this, style);
    if (isImageType())
        return new (arena) RenderImage(this);
    return new (arena) RenderPartObject(this);
}

void HTMLObjectElementImpl::attach()
{
    HTMLElementImpl::attach();

    if (renderer() && !m_useFallbackContent) {
        if (isImageType()) {
            if (!m_imageLoader)
                m_imageLoader = new HTMLImageLoader(this);
            m_imageLoader->updateFromElement();
            if (renderer()) {
                RenderImage* imageObj = static_cast<RenderImage*>(renderer());
                imageObj->setCachedImage(m_imageLoader->image());
            }
        } else {
            if (needWidgetUpdate) {
                // Set needWidgetUpdate to false before calling updateWidget because updateWidget may cause
                // this method or recalcStyle (which also calls updateWidget) to be called.
                needWidgetUpdate = false;
                static_cast<RenderPartObject*>(renderer())->updateWidget();
            } else {
                needWidgetUpdate = true;
                setChanged();
            }
        }
    }
}

void HTMLObjectElementImpl::closeRenderer()
{
    // The parser just reached </object>.
    setComplete(true);
    
    HTMLElementImpl::closeRenderer();
}

void HTMLObjectElementImpl::setComplete(bool complete)
{
    if (complete != m_complete) {
        m_complete = complete;
        if (complete && inDocument() && !m_useFallbackContent) {
            needWidgetUpdate = true;
            setChanged();
        }
    }
}

void HTMLObjectElementImpl::detach()
{
    if (attached() && renderer() && !m_useFallbackContent) {
        // Update the widget the next time we attach (detaching destroys the plugin).
        needWidgetUpdate = true;
    }

    // Delete objectInstance, because it references the widget owned by the renderer we're about to destroy.
    if (objectInstance) {
        delete objectInstance;
        objectInstance = 0;
    }
    
    HTMLElementImpl::detach();
}

void HTMLObjectElementImpl::insertedIntoDocument()
{
    if (isDocNamedItem() && getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->addNamedItem(oldNameAttr);
        document->addDocExtraNamedItem(oldIdAttr);
    }

    HTMLElementImpl::insertedIntoDocument();
}

void HTMLObjectElementImpl::removedFromDocument()
{
    if (isDocNamedItem() && getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        document->removeNamedItem(oldNameAttr);
        document->removeDocExtraNamedItem(oldIdAttr);
    }

    HTMLElementImpl::removedFromDocument();
}

void HTMLObjectElementImpl::recalcStyle(StyleChange ch)
{
    if (!m_useFallbackContent && needWidgetUpdate && renderer() && !isImageType()) {
        detach();
        attach();
    }
    HTMLElementImpl::recalcStyle(ch);
}

void HTMLObjectElementImpl::childrenChanged()
{
    updateDocNamedItem();
    if (inDocument() && !m_useFallbackContent) {
        needWidgetUpdate = true;
        setChanged();
    }
}

bool HTMLObjectElementImpl::isURLAttribute(AttributeImpl *attr) const
{
    return (attr->name() == dataAttr || (attr->name() == usemapAttr && attr->value().domString()[0] != '#'));
}

bool HTMLObjectElementImpl::isImageType()
{
    if (serviceType.isEmpty() && url.startsWith("data:")) {
        // Extract the MIME type from the data URL.
        int index = url.find(';');
        if (index == -1)
            index = url.find(',');
        if (index != -1) {
            int len = index - 5;
            if (len > 0)
                serviceType = url.mid(5, len);
            else
                serviceType = "text/plain"; // Data URLs with no MIME type are considered text/plain.
        }
    }
    
    return Image::supportsType(serviceType);
}

void HTMLObjectElementImpl::renderFallbackContent()
{
    if (m_useFallbackContent)
        return;

    // Mark ourselves as using the fallback content.
    m_useFallbackContent = true;

    // Now do a detach and reattach.    
    // FIXME: Style gets recalculated which is suboptimal.
    detach();
    attach();
}

void HTMLObjectElementImpl::updateDocNamedItem()
{
    // The rule is "<object> elements with no children other than
    // <param> elements and whitespace can be found by name in a
    // document, and other <object> elements cannot."
    bool wasNamedItem = m_docNamedItem;
    bool isNamedItem = true;
    NodeImpl *child = firstChild();
    while (child && isNamedItem) {
        if (child->isElementNode()) {
            if (!static_cast<ElementImpl *>(child)->hasTagName(paramTag))
                isNamedItem = false;
        } else if (child->isTextNode()) {
            if (!static_cast<TextImpl *>(child)->containsOnlyWhitespace())
                isNamedItem = false;
        } else
            isNamedItem = false;
        child = child->nextSibling();
    }
    if (isNamedItem != wasNamedItem && getDocument()->isHTMLDocument()) {
        HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
        if (isNamedItem) {
            document->addNamedItem(oldNameAttr);
            document->addDocExtraNamedItem(oldIdAttr);
        } else {
            document->removeNamedItem(oldNameAttr);
            document->removeDocExtraNamedItem(oldIdAttr);
        }
    }
    m_docNamedItem = isNamedItem;
}

DOMString HTMLObjectElementImpl::code() const
{
    return getAttribute(codeAttr);
}

void HTMLObjectElementImpl::setCode(const DOMString &value)
{
    setAttribute(codeAttr, value);
}

DOMString HTMLObjectElementImpl::align() const
{
    return getAttribute(alignAttr);
}

void HTMLObjectElementImpl::setAlign(const DOMString &value)
{
    setAttribute(alignAttr, value);
}

DOMString HTMLObjectElementImpl::archive() const
{
    return getAttribute(archiveAttr);
}

void HTMLObjectElementImpl::setArchive(const DOMString &value)
{
    setAttribute(archiveAttr, value);
}

DOMString HTMLObjectElementImpl::border() const
{
    return getAttribute(borderAttr);
}

void HTMLObjectElementImpl::setBorder(const DOMString &value)
{
    setAttribute(borderAttr, value);
}

DOMString HTMLObjectElementImpl::codeBase() const
{
    return getAttribute(codebaseAttr);
}

void HTMLObjectElementImpl::setCodeBase(const DOMString &value)
{
    setAttribute(codebaseAttr, value);
}

DOMString HTMLObjectElementImpl::codeType() const
{
    return getAttribute(codetypeAttr);
}

void HTMLObjectElementImpl::setCodeType(const DOMString &value)
{
    setAttribute(codetypeAttr, value);
}

DOMString HTMLObjectElementImpl::data() const
{
    return getAttribute(dataAttr);
}

void HTMLObjectElementImpl::setData(const DOMString &value)
{
    setAttribute(dataAttr, value);
}

bool HTMLObjectElementImpl::declare() const
{
    return !getAttribute(declareAttr).isNull();
}

void HTMLObjectElementImpl::setDeclare(bool declare)
{
    setAttribute(declareAttr, declare ? "" : 0);
}

DOMString HTMLObjectElementImpl::height() const
{
    return getAttribute(heightAttr);
}

void HTMLObjectElementImpl::setHeight(const DOMString &value)
{
    setAttribute(heightAttr, value);
}

DOMString HTMLObjectElementImpl::hspace() const
{
    return getAttribute(hspaceAttr);
}

void HTMLObjectElementImpl::setHspace(const DOMString &value)
{
    setAttribute(hspaceAttr, value);
}

DOMString HTMLObjectElementImpl::name() const
{
    return getAttribute(nameAttr);
}

void HTMLObjectElementImpl::setName(const DOMString &value)
{
    setAttribute(nameAttr, value);
}

DOMString HTMLObjectElementImpl::standby() const
{
    return getAttribute(standbyAttr);
}

void HTMLObjectElementImpl::setStandby(const DOMString &value)
{
    setAttribute(standbyAttr, value);
}

int HTMLObjectElementImpl::tabIndex() const
{
    return getAttribute(tabindexAttr).toInt();
}

void HTMLObjectElementImpl::setTabIndex(int tabIndex)
{
    setAttribute(tabindexAttr, QString::number(tabIndex));
}

DOMString HTMLObjectElementImpl::type() const
{
    return getAttribute(typeAttr);
}

void HTMLObjectElementImpl::setType(const DOMString &value)
{
    setAttribute(typeAttr, value);
}

DOMString HTMLObjectElementImpl::useMap() const
{
    return getAttribute(usemapAttr);
}

void HTMLObjectElementImpl::setUseMap(const DOMString &value)
{
    setAttribute(usemapAttr, value);
}

DOMString HTMLObjectElementImpl::vspace() const
{
    return getAttribute(vspaceAttr);
}

void HTMLObjectElementImpl::setVspace(const DOMString &value)
{
    setAttribute(vspaceAttr, value);
}

DOMString HTMLObjectElementImpl::width() const
{
    return getAttribute(widthAttr);
}

void HTMLObjectElementImpl::setWidth(const DOMString &value)
{
    setAttribute(widthAttr, value);
}

// -------------------------------------------------------------------------

HTMLParamElementImpl::HTMLParamElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(paramTag, doc)
{
}

HTMLParamElementImpl::~HTMLParamElementImpl()
{
}

void HTMLParamElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == idAttr) {
        // Must call base class so that hasID bit gets set.
        HTMLElementImpl::parseMappedAttribute(attr);
        if (getDocument()->htmlMode() != DocumentImpl::XHtml)
            return;
        m_name = attr->value();
    } else if (attr->name() == nameAttr) {
        m_name = attr->value();
    } else if (attr->name() == valueAttr) {
        m_value = attr->value();
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

bool HTMLParamElementImpl::isURLAttribute(AttributeImpl *attr) const
{
    if (attr->name() == valueAttr) {
        AttributeImpl *attr = attributes()->getAttributeItem(nameAttr);
        if (attr) {
            DOMString value = attr->value().domString().lower();
            if (value == "src" || value == "movie" || value == "data")
                return true;
        }
    }
    return false;
}

void HTMLParamElementImpl::setName(const DOMString &value)
{
    setAttribute(nameAttr, value);
}

DOMString HTMLParamElementImpl::type() const
{
    return getAttribute(typeAttr);
}

void HTMLParamElementImpl::setType(const DOMString &value)
{
    setAttribute(typeAttr, value);
}

void HTMLParamElementImpl::setValue(const DOMString &value)
{
    setAttribute(valueAttr, value);
}

DOMString HTMLParamElementImpl::valueType() const
{
    return getAttribute(valuetypeAttr);
}

void HTMLParamElementImpl::setValueType(const DOMString &value)
{
    setAttribute(valuetypeAttr, value);
}

}
