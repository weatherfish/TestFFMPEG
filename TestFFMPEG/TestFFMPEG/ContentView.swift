//
//  ContentView.swift
//  TestFFMPEG
//
//  Created by Felix on 2024/6/12.
//

import SwiftUI
//import "ReSample.h"

struct ContentView: View {
    @State private var isRecording = false

      var body: some View {
          VStack {
              Button(isRecording ? "Stop Recording" : "Start Recording") {
                  // 点击按钮时，切换录音状态
                  isRecording.toggle()
                  
                  // 根据当前的录音状态调用相应的函数
                  if isRecording {
                      recAudio()
                  } else {
                      setStatus(0)
                  }
              }
              .padding()
              .cornerRadius(10)
          }
          .padding()
      }
}

#Preview {
    ContentView()
}
