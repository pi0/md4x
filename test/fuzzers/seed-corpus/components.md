:icon-star

:badge[New]{color="blue"}

:tooltip{text="Hover me"}

:badge[**Bold** content]{.highlight #main}

::alert{type="info"}
This is **important** content with [a link](https://example.com).
::

::card{title="My Card" .bordered}

---

icon: mdi:star
to: /page

---

Default slot content.

#header

## Card Header

#footer
Footer text
::

:::outer
::inner{nested=true}
Inner content
::
:::
