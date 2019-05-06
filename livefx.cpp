#include <stdio.h>

#ifdef __linux
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h> 
#elif __WIN32
// TODO
#endif

#include <SDL2/SDL.h>
#include <GL/gl.h>

#include "livefx_opengl_types.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;
typedef u8 b8;

typedef float f32;
typedef double f64;

#define internal static
#define local_persist static
#define global static

#define ASSERT(X) if(!(X)) { printf("ASSERT: %s:%d \n", __FILE__, __LINE__); __builtin_trap(); }
#define ARRAYCOUNT(A) sizeof(A) / sizeof(A[0])

#define PROC(Type, Name) global Type *Name
#include "livefx_opengl_functions.h"
#undef PROC

GLuint
CreateShader(GLenum ShaderType, char *Source) {
    GLuint Result = glCreateShader(ShaderType);
    glShaderSource(Result, 1, (const char **)&Source, 0);
    glCompileShader(Result);
    
    GLint ShaderCompileSuccess;
    
    glGetShaderiv(Result, GL_COMPILE_STATUS, &ShaderCompileSuccess);
    GLsizei StrLength;
    GLchar MessageLog[1024];
    snprintf(MessageLog, sizeof(MessageLog), "No OpenGL error message\n");
    glGetShaderInfoLog(Result, 1024, &StrLength, MessageLog);
    if(!ShaderCompileSuccess) {
        printf("ERROR: Shader Compilation Error: %s", (char *)MessageLog);
        return 0;
    }
    
    return Result;
}

s32
LinkProgram(GLint ShaderProgram) {
    glLinkProgram(ShaderProgram);
    GLint Linked;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Linked);
    if(!Linked) {
        printf("ERROR: Shader did not link\n");
        
        GLsizei StrLength;
        GLchar MessageLog[1024];
        glGetShaderInfoLog(ShaderProgram, 1024, &StrLength, MessageLog);
        printf("%s\n", (char *)MessageLog);
        
        glGetProgramInfoLog(ShaderProgram, 1024, &StrLength, MessageLog);
        printf("%s\n", (char *)MessageLog);
        return 0;
    }
    
    return 1;
}

s32
UpdateShaderProgramFragShader(GLint ShaderProgram, char *FragSource) {
    GLuint FragShader = CreateShader(GL_FRAGMENT_SHADER, FragSource);
    glAttachShader(ShaderProgram, FragShader);
    s32 LinkSuccess = LinkProgram(ShaderProgram);
    glDetachShader(ShaderProgram, FragShader);
    glDeleteShader(FragShader);
    
    return LinkSuccess && FragShader;
}

GLuint
CreateShaderProgram(char *FragSource, GLuint VertexShader) {
    GLuint FragShader = CreateShader(GL_FRAGMENT_SHADER, FragSource);
    
    if(!FragShader) {
        return 0;
    }
    
    GLuint Program = glCreateProgram();
    if(!Program) {
        // I don't even know what to do about this one..
        // If this happens it's probably because initalizing OpenGL failed.
        printf("ERROR: glCreateProgram failed\n");
    }
    
    glAttachShader(Program, FragShader);
    glAttachShader(Program, VertexShader);
    
    glLinkProgram(Program);
    GLint Linked;
    glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
    if(!Linked) {
        printf("ERROR: Shader did not link\n");
        
        GLsizei StrLength;
        GLchar MessageLog[1024];
        glGetShaderInfoLog(Program, 1024, &StrLength, MessageLog);
        printf("%s\n", (char *)MessageLog);
        
        glGetProgramInfoLog(Program, 1024, &StrLength, MessageLog);
        printf("%s\n", (char *)MessageLog);
        return 0;
    }
    
    glDetachShader(Program, FragShader);
    glDeleteShader(FragShader);
    return Program;
}

#ifdef __linux

int
OpenFile(char *FilePath, int *FileDescriptor) {
    int FD = open(FilePath, O_RDONLY);
    if(FD == -1) {
        // TODO(Redab): Replace strerror() with something that writes to a buffer owned
        // by us in case we end up not wanting to quit the application if file open fails.
        printf("ERROR: Opening file %s failed: %s\n", FilePath, strerror(errno));
        return 0;
    }
    
    *FileDescriptor = FD;
    return 1;
}

struct shader_source {
    char *Source;
    u32 Length;
};

s32
LoadShaderSource(char *FilePath, shader_source *Out, time_t *LastModified) {
    struct stat FileStat;
    int FD;
    if(!OpenFile(FilePath, &FD)) {
        printf("ERROR: Opening file failed: %s\n", FilePath);
        return 0;
    }
    
    int FileStatus = fstat(FD, &FileStat);
    auto FileSize = FileStat.st_size;
    char *Source = (char *)malloc(FileSize + 1);
    ssize_t BytesRead = read(FD, (void *)Source, FileSize);
    Source[FileSize] = 0;
    ASSERT(BytesRead == FileSize);
    Out->Length = FileSize,
    Out->Source = Source;
    *LastModified = FileStat.st_mtime;
    return 1;
}

// TODO(Redab): Can we keep a handle to the source file?
s32
FileChanged(char *FilePath, time_t *Time) {
    s32 Result = 0;
    int FD;
    if(!OpenFile(FilePath, &FD)) {
        return 0;
    }
    
    struct stat FileStat;
    int FileStatus = fstat(FD, &FileStat);
    if(FileStat.st_mtime > *Time) {
        Result = 1;
        *Time = FileStat.st_mtime;
    }
    
    close(FD);
    
    return Result;
}

void
Free(void *Ptr) {
    free(Ptr);
}

#elif __WIN32
#endif

void
Exit() {
    printf("Bye!\n");
    exit(0);
}

// Returns bytes written
// Src is null terminated
u32
CopyString(char *Dest, char *Src, u32 DestSize, b32 WriteNullTerminator = true) {
    u32 Count = 0;
    char *S = Src;
    char *D = Dest;
    
    u32 End = WriteNullTerminator ? DestSize - 1 : DestSize;
    while(*S && Count < End) {
        Count++;
        *Dest++ = *S++;
    }
    
    if(WriteNullTerminator) {
        *Dest = 0;
        Count++;
    }
    
    return Count;
}

// Returns bytes written to dest
// Buf contains StringCount number of null terminated strings
u32
ConcatenateStringBuffer(char *Dest, char **Buf, u32 DestSize, u32 StringCount) {
    u32 Count = 0;
    for(int i = 0; i < StringCount; i++) {
        Count += CopyString(Dest + Count, Buf[i], DestSize - Count, false);
        ASSERT(Count < DestSize);
    }
    
    Dest[Count++] = 0;
    
    return Count;
}

u32
StringLength(char *Str) {
    char *C = Str;
    u32 Count = 0;
    while(*C) {
        C++;
        Count++;
    }
    
    return Count;
}

void
Concatenate(char *Dest, char *A, char *B) {
    while(*A) { 
        *Dest++ = *A++;
    }
    
    while(*B) {
        *Dest++ = *B++;
    }
    *Dest = 0;
}

int
main(int ArgCount, char **Args) {
    if(ArgCount != 2) {
        printf("USAGE: livefx <source file path>\n");
        Exit();
    }
    
    char *FragFilePath = Args[1];
    
    u32 WindowWidth = 640;
    u32 WindowHeight = 480;
    
    SDL_Window *Window;
    SDL_Init(SDL_INIT_VIDEO);

    Window = SDL_CreateWindow("livefx",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WindowWidth,
                              WindowHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!Window) {
        printf("ERROR: SDL window creation failed: %s\n", SDL_GetError());
        Exit();
    }
    
    SDL_GLContext GLContext = SDL_GL_CreateContext(Window);
#define PROC(Type, Name) Name = (Type *)SDL_GL_GetProcAddress(#Name); ASSERT(Name);
#include "livefx_opengl_functions.h"
#undef PROC
    
    // http://docs.gl/gl4/glGetString
    // "The GL_VERSION and GL_SHADING_LANGUAGE_VERSION strings begin with a version number. 
    // The version number uses one of these forms:
    // major_number.minor_number major_number.minor_number.release_number"
    char *VersionString = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("INFO: GLSL version: %s\n", VersionString);
    
    s32 Major = (s32)(VersionString[0]) - '0';
    s32 Minor = (s32)(VersionString[2]) - '0';
    
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, Major) < 0) {
        printf("ERROR: Setting SDL GL attribute major version number to %d failed.\n", Major);
        Exit();
    }
    
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, Minor) < 0) {
        printf("ERROR: Setting SDL GL attribute minor version number to %d failed.\n", Minor);
        Exit();
    }
    
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
        printf("ERROR: Core GL context profile requested and failed: %s\n", SDL_GetError());
        Exit();
    }
    
    s32 ContextProfile;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &Major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &Minor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &ContextProfile);
    char *ContextProfileStr;
    
    switch(ContextProfile) {
        case SDL_GL_CONTEXT_PROFILE_CORE: {
            ContextProfileStr = "core";
        } break;
        
        case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY: {
            ContextProfileStr = "compatability";
        } break;
        
        case SDL_GL_CONTEXT_PROFILE_ES: {
            ContextProfileStr = "es";
        } break;
        
        default: {
            if(ContextProfile == 0) {
                ContextProfileStr = "platform specific default profile";
            } else {
                ContextProfileStr = "profile unknowm";
            }
        } break;
    }
    printf("INFO: OpenGL context version (should match detected version): %d.%d, %s \n", Major, Minor, ContextProfileStr);
    
    char GLSLVersion[5] = {VersionString[0], VersionString[2], '0', ' ', 0};
    
    // TODO(Redab): Can we spawn contexts for all versions of OpenGL we want to support and 
    // try to compile the vertex shader on all of them?
    GLuint VertexShader;
    {
        char VertexCode[1028];
        char *Source = R"FOO(
            //precision mediump float;
            attribute vec2 Pos;
            varying vec2 FragPosition;
            uniform vec2 Resolution;
            void main() {
                FragPosition = (Pos + vec2(1)) * 0.5 * Resolution;
                gl_Position = vec4(Pos, 0, 1);
            })FOO";
        char *Header[] = {"#version ", GLSLVersion, "\n", Source};
        
        ConcatenateStringBuffer(VertexCode, Header, sizeof(VertexCode), ARRAYCOUNT(Header));
        ASSERT(VertexShader = CreateShader(GL_VERTEX_SHADER, Source));
    }
    
    time_t ShaderModifiedTime = {};
    char *FragSourceHeader;
    u32 FragSourceHeaderLength = 0;
    GLint ShaderProgram;
    {
        char *Header[] = {"#version ", 
                          GLSLVersion,
                          "\n",
                          "precision mediump float;\n",
                          "in vec2 FragPosition;\n",
                          "uniform vec2 Resolution;\n"
                          "uniform float Time;\n"
                          };
        
        for(int i = 0; i < ARRAYCOUNT(Header); i++) {
            FragSourceHeaderLength += StringLength(Header[i]);
        }
        
        FragSourceHeader = (char *)malloc(FragSourceHeaderLength + 1);
        ConcatenateStringBuffer(FragSourceHeader, Header, FragSourceHeaderLength + 1, ARRAYCOUNT(Header));
        
        shader_source FragSource;
        if(!LoadShaderSource(FragFilePath, &FragSource, &ShaderModifiedTime)) {
            Exit();
        }
        
        char *SourceBuffer = (char *)malloc(FragSourceHeaderLength + FragSource.Length + 1);
        Concatenate(SourceBuffer, FragSourceHeader, FragSource.Source);
        Free(FragSource.Source);
        
        ShaderProgram = CreateShaderProgram(SourceBuffer, VertexShader);
        if(!ShaderProgram) {
            Exit();
        }
        
        Free(SourceBuffer);
    }
    
    GLuint VBO;
    {
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        struct {f32 X; f32 Y;} V[4] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
        glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    b32 ShaderProgramWorking = true;

    f32 Time = 0;
    u64 PerformanceFrequency = SDL_GetPerformanceFrequency();
    u64 PerformanceCounter = SDL_GetPerformanceCounter();
    while(1) {
        if(FileChanged(FragFilePath, &ShaderModifiedTime)) {
            shader_source FragSource = {};
            if(LoadShaderSource(FragFilePath, &FragSource, &ShaderModifiedTime)) {
                char *SourceBuffer = (char *) malloc(FragSourceHeaderLength + FragSource.Length + 1);
                Concatenate(SourceBuffer, FragSourceHeader, FragSource.Source);
                Free(FragSource.Source);
                ShaderProgramWorking = UpdateShaderProgramFragShader(ShaderProgram, SourceBuffer);
                if(ShaderProgramWorking) {
                    printf("INFO: Shader reloaded!\n");
                } 
                Free(SourceBuffer);
            }
        }
        
        SDL_Event Event;
        while(SDL_PollEvent(&Event)) {
            if(Event.type == SDL_QUIT) {
                printf("Bye\n");
                return 0;
            } else if(Event.type == SDL_WINDOWEVENT) {
                switch(Event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        WindowWidth = Event.window.data1;
                        WindowHeight = Event.window.data2;
                        glViewport(0, 0, WindowWidth, WindowHeight);
                    } break;
                }
            } else if(Event.type == SDL_KEYDOWN) {
                switch(Event.key.keysym.sym) {
                    case SDLK_ESCAPE: {
                        return 0;
                    } break;
                }
            }
        }
        
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        if(ShaderProgramWorking) {
            glUseProgram(ShaderProgram);

            glUniform2f(glGetUniformLocation(ShaderProgram, "Resolution"), (f32)WindowWidth, (f32)WindowHeight);
            glUniform1f(glGetUniformLocation(ShaderProgram, "Time"), Time);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            
            GLint PosAttribLoc = glGetAttribLocation(ShaderProgram, "Pos");
            glEnableVertexAttribArray(PosAttribLoc);
            glVertexAttribPointer(PosAttribLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glUseProgram(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else {
            glClearColor(1, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        
        SDL_GL_SwapWindow(Window);

        u64 EndCounter = SDL_GetPerformanceCounter();
        Time += (f32)(EndCounter - PerformanceCounter) / PerformanceFrequency;
        PerformanceCounter = EndCounter;
    }
    return 0;
}
