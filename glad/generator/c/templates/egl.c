{% extends 'base_template.c' %}

{% block loader %}
static int get_exts(EGLDisplay display, const char **extensions) {
    *extensions = eglQueryString(display, EGL_EXTENSIONS);

    return extensions != NULL;
}

static int has_ext(const char *extensions, const char *ext) {
    const char *loc;
    const char *terminator;
    if(extensions == NULL) {
        return 0;
    }
    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static GLADapiproc glad_egl_get_proc_from_userptr(const char *name, void *userptr) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
static int find_extensions{{ api|api }}(EGLDisplay display) {
    const char *extensions;
    if (!get_exts(display, &extensions)) return 0;

{% for extension in feature_set.extensions %}
    GLAD_{{ extension.name }} = has_ext(extensions, "{{ extension.name }}");
{% else %}
    (void)has_ext;
{% endfor %}

    return 1;
}

static int find_core{{ api|api }}(EGLDisplay display) {
    int major, minor;
    const char *version;

    if (display == NULL) {
        display = EGL_NO_DISPLAY; /* this is usually NULL, better safe than sorry */
    }
    if (display == EGL_NO_DISPLAY) {
        display = eglGetCurrentDisplay();
    }
    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (display == EGL_NO_DISPLAY) {
        return 0;
    }

    version = eglQueryString(display, EGL_VERSION);
    (void) eglGetError();

    if (version == NULL) {
        major = 1;
        minor = 0;
    } else {
        GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);
    }

{% for feature in feature_set.features %}
    GLAD_{{ feature.name }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
{% endfor %}

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoad{{ api|api }}UserPtr(EGLDisplay display, GLADuserptrloadfunc load, void* userptr) {
    int version;
    eglGetDisplay = (PFNEGLGETDISPLAYPROC) load("eglGetDisplay", userptr);
    eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) load("eglGetCurrentDisplay", userptr);
    eglQueryString = (PFNEGLQUERYSTRINGPROC) load("eglQueryString", userptr);
    eglGetError = (PFNEGLGETERRORPROC) load("eglGetError", userptr);
    if (eglGetDisplay == NULL || eglGetCurrentDisplay == NULL || eglQueryString == NULL || eglGetError == NULL) return 0;

    version = find_core{{ api|api }}(display);
    if (!version) return 0;
{% for feature, _ in loadable(feature_set.features) %}
    load_{{ feature.name }}(load, userptr);
{% endfor %}

    if (!find_extensions{{ api|api }}(display)) return 0;
{% for extension, _ in loadable(feature_set.extensions) %}
    load_{{ extension.name }}(load, userptr);
{% endfor %}

    return version;
}

int gladLoad{{ api|api }}(EGLDisplay display, GLADloadfunc load) {
    return gladLoad{{ api|api }}UserPtr(display, glad_egl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}
{% endfor %}

{% endblock %}